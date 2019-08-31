#include "deps.h"

namespace Shaders {

using namespace Textures;


static const char* vs_src = R"(
#version 450
layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vUv;

out vec3 pos;
out vec3 normal;
out vec2 uv;

layout(location = 0) uniform mat4 uCamera;
layout(location = 1) uniform mat4 uMvp;

void main() {
  vec4 worldPos = uMvp * vec4(vPos, 1); 
  gl_Position = uCamera * worldPos;
  pos = worldPos.xyz;
  normal = normalize(uMvp * vec4(vNormal, 0)).xyz;
  uv = vUv;
}
)";

static const char* fs_src = R"(
#version 450

in vec3 pos;
in vec3 normal;
in vec2 uv;

out vec3 c_pos;
out vec3 c_normal;
out vec3 c_material;

void main() {
  c_pos = (pos + 1) / 2;
  c_normal = (normal + 1) / 2;
  c_material = vec3(1);
}
)";

static const char* quad_vs_src = R"( #version 450
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

static const char* defer_fs_src = R"(
#version 450

in vec2 uv;

layout(location = 15) uniform sampler2D t_pos;
layout(location = 16) uniform sampler2D t_normal;
layout(location = 17) uniform sampler2D t_material;

out vec3 color;

layout (std140) uniform shader_data
{
  vec4 light_pos[32];
  vec4 light_col[32];
};

void main() {
  vec3 pos = texture(t_pos, uv).xyz * 2 - 1;
  vec3 normal = normalize(texture(t_normal, uv).xyz * 2 - 1);
  vec3 material = texture(t_material, uv).xyz;
  vec3 C = vec3(0);
  color = vec3(0);
  for(int i=0; i<32; i++) {
    vec3 lVec = pos - light_pos[i].xyz;
    float dist = length(lVec);
    vec3 lDir = lVec / dist;
    float falloff = 1 / dist;
    vec3 E = normalize(C - light_pos[i].xyz);
    vec3 R = reflect(-lDir, normal);
    vec3 specular = material * light_col[i].xyz * pow(max(dot(E, R), 0), 50) * falloff;
    color += material * light_col[i].xyz * max(dot(lDir, normal), 0) * falloff;
    color += specular;
  }
  if (gl_FragCoord.x < 0.5)
    color = vec3(0.2, 0.2, 1);
}
)";

GLuint loadShaderLiteral(const char* vs, const char* fs) {
    return GenerateProgram(
        CompileShader(GL_VERTEX_SHADER, vs),
        CompileShader(GL_FRAGMENT_SHADER, fs));
}

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
  void use(const Matrix4 &camera, const Matrix4 &mvp) const {
    glUseProgram(program_id);
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);

    mat4x4 m_mvp;
    mvp.unpack(m_mvp);
    glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_mvp);
  }
} sh_main;

struct sh_combinator_t {
  // Combination shader that combines to the g buffers to a quad
  GLuint program_id;
  GLuint ubo;
  void use(const lights_t &lights, Texture g_pos, Texture g_norm, Texture g_mat) const {
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(lights), &lights, GL_DYNAMIC_DRAW);
    glUseProgram(program_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_pos);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_norm);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_mat);
  }
} sh_combinator;

void init() {
  printf("Shaders are being initalized\n");

  // QUAD SHADER
  sh_quad.program_id = loadShaderLiteral(quad_vs_src, quad_fs_src);
  // MAIN SHADER
  sh_main.program_id = loadShaderLiteral(vs_src, fs_src);


  // COMBINATOR
  sh_combinator.program_id = loadShaderLiteral(quad_vs_src, defer_fs_src); 
  glUseProgram(sh_combinator.program_id);

  // g buffer bindings
  glUniform1i(D_POS_GTEXTURE_INDEX,      0);
  glUniform1i(D_NORMAL_GTEXTURE_INDEX,   1);
  glUniform1i(D_MATERIAL_GTEXTURE_INDEX, 2);

  // lights
  GLuint u_shader_data = glGetUniformBlockIndex(sh_combinator.program_id, "shader_data"); 
  glGenBuffers(1, &sh_combinator.ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, sh_combinator.ubo);
  glBindBufferBase(GL_UNIFORM_BUFFER, u_shader_data, sh_combinator.ubo);
  // END COMBINATOR
}

void setMVP(const Matrix4 &m) {
  mat4x4 mm;
  m.unpack(mm);
  glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)mm);
}

void setCamera(const Matrix4 &m) {
  mat4x4 mm;
  m.unpack(mm);
  glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)mm);
}

}


