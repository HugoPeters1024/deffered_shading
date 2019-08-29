#include <stdio.h>
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h>

#include <vector>

#include "gl_debug.h"
#include "shader.h"
#include "vec.h"
#include "obj_loader.h"

static const char* vs_src = R"(
#version 450
in vec3 vPos;
in vec3 vNormal;
in vec2 vUv;

out vec3 pos;
out vec3 normal;
out vec2 uv;

uniform mat4 uMvp;
uniform mat4 uCamera;

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

out vec3 color;
out vec3 c_normal;

void main() {
  color = vec3(1) + normal * 0.00001 * uv.x * 0.000001 * uv.y;
  c_normal = normal / 2 + 0.5;
}
)";

static const char* quad_vs_src = R"(
#version 450
in vec3 vPos;

out vec2 uv;
out vec3 pos;

void main() {
 gl_Position = vec4(vPos, 1);
 uv = vPos.xy/2 + 0.5;
 pos = vPos;
}
)";

static const char* quad_fs_src = R"(
#version 450

in vec2 uv;
in vec3 pos;

out vec3 color;

uniform sampler2D tex;

void main() {
  color = texture(tex, uv).xyz;
}
)";

void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char** argv) {
  if (!glfwInit()) return 2;
  glfwSetErrorCallback(glfw_error_callback);
  glEnable(GL_DEBUG_OUTPUT);

	glDebugMessageCallback(GLDEBUGPROC(gl_debug_output), nullptr); glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

  GLFWwindow* window = glfwCreateWindow(640, 480, "Yeah", NULL, NULL);
  if (!window) return 3;
  glfwMakeContextCurrent(window);

  // Create render target
  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  GLuint posTex;
  glGenTextures(1, &posTex);
  glBindTexture(GL_TEXTURE_2D, posTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, posTex, 0);

  GLuint normalTex;
  glGenTextures(1, &normalTex);
  glBindTexture(GL_TEXTURE_2D, normalTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normalTex, 0);

  GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, DrawBuffers);

  // Always check that our framebuffer is ok
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    exit(7);

  // Initialize the main scene
  GLuint program = GenerateProgram(CompileShader(GL_VERTEX_SHADER, vs_src), CompileShader(GL_FRAGMENT_SHADER, fs_src));
  GLuint uMvp = glGetUniformLocation(program, "uMvp");
  GLuint uCamera = glGetUniformLocation(program, "uCamera");

  cObj* model = new cObj("stack.obj");
  std::vector<float> vertices, normals, uvs;
  model->renderBuffers(vertices, normals, uvs);
  delete model;

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo, nbo, uvbo;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &nbo);
  glGenBuffers(1, &uvbo);

  // Fill vertices
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
  GLuint vPos = glGetAttribLocation(program, "vPos");
  glEnableVertexAttribArray(vPos);
  glVertexAttribPointer(vPos, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 0));

  // Fill normals
  glBindBuffer(GL_ARRAY_BUFFER, nbo);
  glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(float), normals.data(), GL_STATIC_DRAW);
  GLuint vNormal = glGetAttribLocation(program, "vNormal");
  glEnableVertexAttribArray(vNormal);
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 0));
  
  // Fill uvs
  glBindBuffer(GL_ARRAY_BUFFER, uvbo);
  glBufferData(GL_ARRAY_BUFFER, uvs.size()*sizeof(float), uvs.data(), GL_STATIC_DRAW);
  GLuint vUv = glGetAttribLocation(program, "vUv");
  glEnableVertexAttribArray(vUv);
  glVertexAttribPointer(vUv, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 0));

  glBindVertexArray(0);

  // Initialize the final quad
  GLuint quad_program = GenerateProgram(CompileShader(GL_VERTEX_SHADER, quad_vs_src), CompileShader(GL_FRAGMENT_SHADER, quad_fs_src));

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

  while(!glfwWindowShouldClose(window))
  {
    int w, h;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glfwGetFramebufferSize(window, &w, &h);
    glUseProgram(program);
    mat4x4 u_mvp, u_camera;
    Matrix4 mvp = Matrix4::FromAxisRotations(0, glfwGetTime(), 0);
    Matrix4 camera = Matrix4::FromPerspective(1.25f, w/h, 0.1f, 1000.0f) * Matrix4::FromTranslation(0, 0, -6);
    mvp.unpack(u_mvp);
    camera.unpack(u_camera);

    glUniformMatrix4fv(uMvp, 1, GL_FALSE, (const GLfloat*)u_mvp);
    glUniformMatrix4fv(uCamera, 1, GL_FALSE, (const GLfloat*)u_camera);

    glViewport(0, 0, w, h); 
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwGetFramebufferSize(window, &w, &h);


    glUseProgram(quad_program);
    glBindVertexArray(quad_vao);
    glBindTexture(GL_TEXTURE_2D, posTex);
    glViewport(0, 0, w/2, h/2);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindTexture(GL_TEXTURE_2D, normalTex);
    glViewport(w/2, h/2, w/2, h/2);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

