#include "deps.h"

namespace Shaders {

using namespace Textures;


static const char* vs_src = R"(
#version 450
layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vUv;

out vec3 position;
out vec3 normal;
out vec2 uv;

layout(location = 0) uniform mat4 uCamera;
layout(location = 1) uniform mat4 uMvp;

void main() {
  vec4 worldPos = uMvp * vec4(vPos, 1); 
  gl_Position = uCamera * worldPos;
  position = worldPos.xyz;
  normal = normalize(uMvp * vec4(vNormal, 0)).xyz;
  uv = vUv;
}
)";

static const char* fs_src = R"(
#version 450

in vec3 position;
in vec3 normal;
in vec2 uv;

out vec3 c_pos;
out vec3 c_normal;
out vec3 c_material;

void main() {
  c_pos = position;
  c_normal = normal;
  c_material = vec3(1);
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

uniform sampler2D tex;

void main() {
 color = texture(tex, uv).xyz;
}

)";

static const char* defer_fs_src = R"(
#version 450

in vec2 uv;

layout(location = 0)  uniform mat4      uCamera;
layout(location = 2)  uniform vec3      uCamPos;
layout(location = 15) uniform sampler2D t_pos;
layout(location = 16) uniform sampler2D t_normal;
layout(location = 17) uniform sampler2D t_material;
layout(location = 18) uniform sampler2D t_depth;

out vec3 color;

layout (std140) uniform shader_data
{
  vec4 light_pos[32];
  vec4 light_col[32];
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

  // ambient
  color = material * 0.2;
  for(int i=0; i<32; i++) {
    vec3 lVec = light_pos[i].xyz - pos;
    float dist2 = dot(lVec, lVec);
    vec3 lDir = lVec / sqrt(dist2);
    float falloff = 1 / dist2;
    color += material * light_col[i].xyz * max(dot(lDir, normal), 0) * falloff;

    vec3 E = normalize(uCamPos - light_pos[i].xyz);
    vec3 R = reflect(-lDir, normal);
    vec3 specular = material * light_col[i].xyz * pow(max(dot(E, R), 0), 20) * falloff;
    color += specular;
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
  void use(const Matrix4 &camera) const {
    glUseProgram(program_id);
    setCamera(camera);
  }
} sh_main;

struct sh_combinator_t {
  // Combination shader that combines to the g buffers to a quad
  GLuint program_id;
  void use(const lights_t &lights, const Matrix4 &camera, const Vector3 &cam_pos, Texture g_pos, Texture g_norm, Texture g_mat, Texture g_depth) const {
    glUseProgram(program_id);
    // Update te light suite
    glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);

    // Populate g buffer slots
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_pos);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_norm);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_mat);
    glActiveTexture(GL_TEXTURE3);
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
  void use(const Texture &tex) const {
    glUseProgram(program_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
} sh_post;

void init() {
  printf("Shaders are being initalized\n");

  // QUAD SHADER
  sh_quad.program_id = loadShaderLiteral(quad_vs_src, quad_fs_src);
  // MAIN SHADER
  sh_main.program_id = loadShaderLiteral(vs_src, fs_src);
  // POST SHADER
  sh_post.program_id = loadShaderLiteral(quad_vs_src, post_fs_src);


  // COMBINATOR
  sh_combinator.program_id = loadShaderLiteral(quad_vs_src, defer_fs_src); 
  glUseProgram(sh_combinator.program_id);

  // g buffer bindings
  glUniform1i(D_POS_GTEXTURE_INDEX,      0);
  glUniform1i(D_NORMAL_GTEXTURE_INDEX,   1);
  glUniform1i(D_MATERIAL_GTEXTURE_INDEX, 2);
  glUniform1i(D_DEPTH_GTEXTURE_INDEX,    3);

  // lights
  glGenBuffers(1, &lights_buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(lights_t), NULL, GL_STREAM_DRAW);

  GLuint u_shader_data = glGetUniformBlockIndex(sh_combinator.program_id, "shader_data"); 
  glBindBufferBase(GL_UNIFORM_BUFFER, u_shader_data, lights_buffer);
  // END COMBINATOR
}

}


