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
  FBO::post_buffer.init();

  Camera::Camera camera = Camera::Camera(1.25);
  camera.setPosition(Vector3(0, 10, 20));

  Keyboards::Keyboard keyboard = Keyboards::Keyboard(window);

  Shaders::init();
  Shaders::lights_t lights;

  auto quad = Meshes::loadMesh("quad.obj");
  auto mesh = Meshes::loadMesh("player.obj");
  auto cube = Meshes::loadMesh("cube.obj");

  while(!glfwWindowShouldClose(window))
  {
    float f = glfwGetTime();
    float x = 16 * sin(f);
    float z = 16 * cos(f);
    float y = 10;
    lights.pos[1] = Vector4(x, y, z, 0);
    lights.col[1] = Vector4(50, 80, 60, 0) * 2;

    int w, h;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);

    camera.update(w/h, &keyboard);

    FBO::g_buffer.bind();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    Matrix4 mvp = Matrix4::FromAxisRotations(0, 0, 0);
    Shaders::sh_main.use(camera.getMatrix());
    Shaders::sh_main.setMvp(mvp);

    glViewport(0, 0, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT); 
    glBindVertexArray(mesh->vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);

    glBindVertexArray(cube->vao);
    Matrix4 cube_mvp = Matrix4::FromTranslation(x, y, z);
    Shaders::sh_main.setMvp(cube_mvp);
    glDrawArrays(GL_TRIANGLES, 0, cube->vertex_count);


    // <--- Draw combined to post buffer ---->
    FBO::post_buffer.bind();
    glViewport(0, 0, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_combinator.use(
        lights,
        camera.getPosition(),
        FBO::g_buffer.posTex,
        FBO::g_buffer.normalTex,
        FBO::g_buffer.materialTex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);
    glBindVertexArray(0);

    // <--- Draw result with pos shader --->
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_quad.use(FBO::post_buffer.tex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);

    keyboard.swapBuffers();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

