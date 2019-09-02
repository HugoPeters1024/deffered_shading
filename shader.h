#include "deps.h"

namespace Shaders {

using namespace Textures;


static const char* vs_src = R"(
#version 450
layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vUv;
layout(location=3) in vec3 vTangent;
layout(location=4) in vec3 vBitangent;

out vec3 position;
out vec3 normal;
out vec2 uv;
out vec3 tangent;
out vec3 bitangent;
out flat mat3 TBN;
out flat int usenormalmap;

layout(location = 0) uniform mat4 uCamera;
layout(location = 1) uniform mat4 uMvp;

void main() {
  vec4 worldPos = uMvp * vec4(vPos, 1); 
  gl_Position = uCamera * worldPos;
  position = worldPos.xyz;
  normal = normalize(uMvp * vec4(vNormal, 0)).xyz;
  tangent = normalize(uMvp * vec4(vTangent, 0)).xyz;
  bitangent = normalize(uMvp * vec4(vBitangent, 0)).xyz;
  uv = vUv;
  if (uv.x != 0 && uv.y != 0) {
    TBN = inverse(mat3(tangent, bitangent, normal));
    usenormalmap = 1;
  }
  else {
    usenormalmap = 0;
  }
}
)";

static const char* fs_src = R"(
#version 450

in vec3 position;
in vec3 normal;
in vec2 uv;
in vec3 tangent;
in vec3 bitangent;
in flat mat3 TBN;
in flat int usenormalmap;
out vec3 c_normal;
out vec3 c_material;

layout(location = 3) uniform float texture_scale;
layout(location = 5) uniform sampler2D material;
layout(location = 6) uniform sampler2D normalmap;

void main() {
  c_material = texture(material, uv * texture_scale).xyz; 
  if (usenormalmap == 1)
    c_normal = normalize((texture(normalmap, uv * texture_scale).xyz * 2 - 1) * TBN); 
  else
    c_normal = normal;
}
)";

static const char* quad_vs_src = R"( 
#version 450
in vec3 vPos;

out vec2 uv;

void main() {
 gl_Position = vec4(vPos, 1);
 uv = vPos.xy/2 + 0.5;
}
)";

static const char* quad_fs_src = R"(
#version 450

in vec2 uv;

out vec3 color;
uniform sampler2D tex;

void main() {
  color = texture(tex, uv).xyz;
}
)";

static const char* post_fs_src = R"(
#version 450

in vec2 uv;
out vec3 color;

layout(location = 0) uniform sampler2D tex;
layout(location = 4) uniform float time;

void main() {
 vec3 map = texture(tex, uv).xyz;
 color = map;
}

)";

static const char* defer_fs_src = R"(
#version 450

in vec2 uv;

layout(location = 0)  uniform mat4      uCamera;
layout(location = 2)  uniform vec3      uCamPos;
layout(location = 15) uniform sampler2D t_normal;
layout(location = 16) uniform sampler2D t_material;
layout(location = 17) uniform sampler2D t_depth;

out vec3 color;

layout (std140) uniform shader_data
{
  vec4 light_pos[32];
  vec4 light_col[32];
  vec4 light_dir[32];
};

// this is supposed to get the world position from the depth buffer
vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverse(uCamera) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main() {
  vec3 normal = texture(t_normal, uv).xyz;
  vec3 material = texture(t_material, uv).xyz;
  float depth = (texture(t_depth, uv)).x;
  vec3 pos = WorldPosFromDepth(depth);
  vec3 E = normalize(uCamPos - pos);

  // ambient
  color = material * 0.2;
  for(int i=0; i<32; i+=1) {
    vec3 lVec = light_pos[i].xyz - pos;
    float dist2 = dot(lVec, lVec);
    vec3 lDir = lVec / sqrt(dist2);
    vec3 lNormal = light_dir[i].xyz;
    float cone = light_dir[i].w;

    //float corr = pow(1 - 0.5*(min(max(abs(cone_angle-cone), 0), 1)), 1000);
     float corr = 1;
    float cone_angle = dot(lDir, -lNormal);
    if ((cone_angle) < cone)
      corr = max(1 - 16 * abs(cone_angle-cone), 0);

    float falloff = 1 / dist2;
    float diffuse = max(dot(lDir, normal), 0);

    vec3 R = reflect(-lDir, normal);
    float specular = pow(max(dot(E, R), 0), 40);
    color += (specular + diffuse) * material * light_col[i].xyz * falloff * corr;
  }
}
)";

static inline GLuint loadShaderLiteral(const char* vs, const char* fs) {
    return GenerateProgram(
        CompileShader(GL_VERTEX_SHADER, vs),
        CompileShader(GL_FRAGMENT_SHADER, fs));
}

static GLuint lights_buffer;

struct lights_t {
  Vector4 pos[32];
  Vector4 col[32];
  Vector4 dir[32]; // xyz for direction, w for cone size (0 normal light)
};

struct sh_quad_t {
  // Simple shader that renders a single texture to a quad
  GLuint program_id;
  void use(const Texture &tex) const  {
    glUseProgram(program_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
} sh_quad;

struct sh_main_t {
  // Main shader that renders to the g buffers
  GLuint program_id;
  void setCamera(const Matrix4 &camera) const {
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
  }
  void setMvp(const Matrix4 &mvp) const {
    mat4x4 m_mvp;
    mvp.unpack(m_mvp);
    glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_mvp);
  }
  void setTextureScale(float s) const {
    glUniform1f(D_TEXTURE_SCALE_UNIFORM_INDEX, s);
  }
  void use(const Matrix4 &camera) const {
    glUseProgram(program_id);
    setCamera(camera);
  }
} sh_main;

struct sh_combinator_t {
  // Combination shader that combines to the g buffers to a quad
  GLuint program_id;
  void use(const lights_t &lights, const Matrix4 &camera, const Vector3 &cam_pos, Texture g_norm, Texture g_mat, Texture g_depth) const {
    glUseProgram(program_id);
    // Update te light suite
    glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);

    // Populate g buffer slots
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_norm);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_mat);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_depth);

    // Populate camera transformation for restoring the fragment position from depth buffer
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
    
    // Populate camera pos for specular lighting
    glUniform3f(D_CAMERAPOS_UNIFORM_INDEX, cam_pos.x, cam_pos.y, cam_pos.z);
  }
} sh_combinator;

struct sh_post_t {
  GLuint program_id;
  void use(const Texture &tex, float time) const {
    glUseProgram(program_id);
    glUniform1f(D_TIME_UNIFORM_INDEX, time);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
} sh_post;

void init() {
  printf("Shaders are being initalized\n");

  // QUAD SHADER
  sh_quad.program_id = loadShaderLiteral(quad_vs_src, quad_fs_src);
  // MAIN SHADER
  logInfo("Compiling main shader");
  sh_main.program_id = loadShaderLiteral(vs_src, fs_src);
  glUseProgram(sh_main.program_id);
  glUniform1i(D_TEXTURE_MATERIAL_INDEX, 0);
  glUniform1i(D_TEXTURE_NORMALMAP_INDEX, 1);
  sh_main.setTextureScale(1);
  logInfo("Main shader compiled succesfully");

  // POST SHADER
  logInfo("Compiling post processing shader");
  sh_post.program_id = loadShaderLiteral(quad_vs_src, post_fs_src);
  logInfo("post processing shader compiled succesfully");


  // COMBINATOR
  logInfo("Compiling combination shader");
  sh_combinator.program_id = loadShaderLiteral(quad_vs_src, defer_fs_src); 
  glUseProgram(sh_combinator.program_id);
  logInfo("Combination shader compiled succesfully");

  // g buffer bindings
  glUniform1i(D_NORMAL_GTEXTURE_INDEX,   0);
  glUniform1i(D_MATERIAL_GTEXTURE_INDEX, 1);
  glUniform1i(D_DEPTH_GTEXTURE_INDEX,    2);

  // lights
  glGenBuffers(1, &lights_buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(lights_t), NULL, GL_STREAM_DRAW);

  GLuint u_shader_data = glGetUniformBlockIndex(sh_combinator.program_id, "shader_data"); 
  glBindBufferBase(GL_UNIFORM_BUFFER, u_shader_data, lights_buffer);
  // END COMBINATOR
}

}


