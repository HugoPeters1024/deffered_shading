#include "deps.h"

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

layout(location = 0) uniform sampler2D t_pos;
layout(location = 1) uniform sampler2D t_normal;
layout(location = 2) uniform sampler2D t_material;

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
}
)";

void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

struct shader_data_t
{
  Vector4 light_pos[32];
  Vector4 light_col[32];
} shader_data;

int main(int argc, char** argv) {
  if (!glfwInit()) return 2;
  glfwSetErrorCallback(glfw_error_callback);
  glEnable(GL_DEBUG_OUTPUT);

	glDebugMessageCallback(GLDEBUGPROC(gl_debug_output), nullptr); 
  glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

  GLFWwindow* window = glfwCreateWindow(640, 480, "Yeah", NULL, NULL);
  if (!window) return 3;
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glEnable(GL_DEPTH_TEST);

  // Create render target
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  // Add depth buffer
  GLuint depthBuf;
  glGenRenderbuffers(1, &depthBuf);
  glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1920, 1080);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

  GLuint posTex;
  glGenTextures(1, &posTex);
  glBindTexture(GL_TEXTURE_2D, posTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, posTex, 0);

  GLuint normalTex;
  glGenTextures(1, &normalTex);
  glBindTexture(GL_TEXTURE_2D, normalTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normalTex, 0);

  GLuint materialTex;
  glGenTextures(1, &materialTex);
  glBindTexture(GL_TEXTURE_2D, materialTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, materialTex, 0);


  GLenum DrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
  glDrawBuffers(3, DrawBuffers);

  // Always check that our framebuffer is ok
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    exit(7);

  // Initialize the main scene
  Shaders::Shader program = Shaders::loadShaderLiteral(vs_src, fs_src);

  // Initialize the final quad
  GLuint quad_program = GenerateProgram(CompileShader(GL_VERTEX_SHADER, quad_vs_src), CompileShader(GL_FRAGMENT_SHADER, quad_fs_src));
  GLuint defer_program = GenerateProgram(CompileShader(GL_VERTEX_SHADER, quad_vs_src), CompileShader(GL_FRAGMENT_SHADER, defer_fs_src));
  GLuint t_pos = glGetUniformLocation(defer_program, "t_pos");
  GLuint t_normal = glGetUniformLocation(defer_program, "t_normal");
  GLuint t_material = glGetUniformLocation(defer_program, "t_material");
  GLuint u_shader_data = glGetUniformBlockIndex(defer_program, "shader_data"); 
  GLuint ubo;
  glGenBuffers(1, &ubo);
  glBindBuffer(GL_UNIFORM_BUFFER, ubo);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(shader_data), &shader_data, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, u_shader_data, ubo);

  float quad_vertices[18] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
  };

  GLuint quad_vao;
  glGenVertexArrays(1, &quad_vao);
  glBindVertexArray(quad_vao);

  GLuint quad_vbo;
  glGenBuffers(1, &quad_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

  GLuint quad_vpos_location = glGetAttribLocation(quad_program, "vPos");
  glEnableVertexAttribArray(quad_vpos_location);
  glVertexAttribPointer(
      quad_vpos_location, 
      3, 
      GL_FLOAT, 
      GL_FALSE,
      0, 
      (void*)0);
  glBindVertexArray(0);

  Meshes::Mesh* mesh = Meshes::loadMesh("player.obj");

  while(!glfwWindowShouldClose(window))
  {
    for(int i=0; i<8; i++) {
      float p = i / 8.0f;
      float f = p * 6.282;
      float x = 16 * sin(f + glfwGetTime());
      float z = 16 * cos(f + glfwGetTime());
      shader_data.light_pos[i] = Vector4(z, z + glfwGetTime(), x, 0);
      shader_data.light_col[i] = 3.0f * Vector4(p, 1-p, -1 + p * 2, 0);
    }

    // Update the light store
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(shader_data), &shader_data);

    int w, h;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    Matrix4 mvp = Matrix4::FromTranslation(0, -10, -25) * Matrix4::FromAxisRotations(0, 0, 0);
    Matrix4 camera = Matrix4::FromPerspective(1.05f, w/h, 0.1f, 1000.0f);

    glUseProgram(program);
    Shaders::setMVP(mvp);
    Shaders::setCamera(camera);

    glViewport(0, 0, 1920, 1080); 
    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwGetFramebufferSize(window, &w, &h);

    // Draw pos
    glActiveTexture(GL_TEXTURE0);
    glUseProgram(quad_program);
    glBindVertexArray(quad_vao);

    /*
    glBindTexture(GL_TEXTURE_2D, posTex);
    glViewport(0, h/2, w/2, h/2);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw normals
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glViewport(w/2, h/2, w/2, h/2);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw material
    glBindTexture(GL_TEXTURE_2D, materialTex);
    glViewport(0, 0, w/2, h/2);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    */

    // <--- Draw combined ---->
    glViewport(0, 0, w, h);

    glUseProgram(defer_program);
    Shaders::initGBufferBinding(program);


    // populate the slots
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, posTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, materialTex);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

