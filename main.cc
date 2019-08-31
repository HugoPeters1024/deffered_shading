#include "deps.h"


void glfw_error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}

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

  FBO::g_buffer.init();

  Shaders::init();
  Shaders::lights_t lights;

  Meshes::Mesh* quad = Meshes::loadMesh("quad.obj");
  Meshes::Mesh* mesh = Meshes::loadMesh("player.obj");

  while(!glfwWindowShouldClose(window))
  {
    for(int i=0; i<8; i++) {
      float p = i / 8.0f;
      float f = p * 6.282;
      float x = 16 * sin(f + glfwGetTime());
      float z = 16 * cos(f + glfwGetTime());
      lights.pos[i] = Vector4(z, z + glfwGetTime(), x, 0);
      lights.col[i] = 5.0f * Vector4(p, 1-p, -1 + p * 2, 0);
    }

    int w, h;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);

    FBO::g_buffer.bind();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    Matrix4 mvp = Matrix4::FromTranslation(0, -10, -25) * Matrix4::FromAxisRotations(0, 0, 0);
    Matrix4 camera = Matrix4::FromPerspective(1.05f, w/h, 0.1f, 1000.0f);

    Shaders::sh_main.use(camera, mvp);

    glViewport(0, 0, 1920, 1080); 
    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwGetFramebufferSize(window, &w, &h);

    // <--- Draw combined ---->
    glViewport(0, 0, w, h);

    Shaders::sh_combinator.use(lights, FBO::g_buffer.posTex, FBO::g_buffer.normalTex, FBO::g_buffer.materialTex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

