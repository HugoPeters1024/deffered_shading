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
  auto floor = Meshes::loadMesh("floor.obj");

  while(!glfwWindowShouldClose(window))
  {
    /*
    float f = glfwGetTime();
    float x = 12 * sin(f);
    float z = 12 * cos(f);
    float y = 10;
    lights.pos[0] = Vector4(-x, y, -z, 0);
    lights.col[0] = Vector4(100, 40, 60, 0);

    lights.pos[1] = Vector4(x, y, z, 0);
    lights.col[1] = Vector4(50, 80, 60, 0);

    lights.pos[2] = Vector4(0, x+10, z, 0);
    lights.col[2] = Vector4(50, 30, 80, 0);

    lights.pos[3] = Vector4(0.5*x,20, 0.5*z, 0);
    lights.col[3] = Vector4(0, 80, 80, 0);
    */

    for(int i=0; i<32; i++) {
      float v = (float)i / 32.0f * 6.28;
      float x = sin(2 * v);
      float z = cos(v);

      lights.pos[i] = Vector4(60 * x, 25, 60 * z, 0);
      lights.col[i] = Vector4(0.5 * x + 0.5, 0.5*z +0.5, 0.5, 1) * 180 * (std::max(sin(2*v + glfwGetTime()*3), 0.0) + 0.5);
    }


    int w, h;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);

    camera.update(w/h, &keyboard);

    FBO::g_buffer.bind();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glViewport(0, 0, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT); 

    Matrix4 mvp = Matrix4::FromAxisRotations(0, 0, 0);
    Shaders::sh_main.use(camera.getMatrix());

    glBindVertexArray(mesh->vao);
    Shaders::sh_main.setMvp(mvp);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);


    glBindVertexArray(cube->vao);
    for(int i=0; i<32; i++) {
      Matrix4 cube_mvp = Matrix4::FromTranslation(lights.pos[i].xyz() + Vector3(0, 8, 0));
      Shaders::sh_main.setMvp(cube_mvp);
      glDrawArrays(GL_TRIANGLES, 0, cube->vertex_count);
    }

    Matrix4 floor_mvp = Matrix4::FromScale(150, 150, 150);
    Shaders::sh_main.setMvp(floor_mvp);
    glBindVertexArray(floor->vao);
    glDrawArrays(GL_TRIANGLES, 0, floor->vertex_count);


    // <--- Draw combined to post buffer ---->
    FBO::post_buffer.bind();
    glViewport(0, 0, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_combinator.use(
        lights,
        camera.getMatrix(),
        camera.getPosition(),
        FBO::g_buffer.posTex,
        FBO::g_buffer.normalTex,
        FBO::g_buffer.materialTex,
        FBO::g_buffer.depthTex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);
    glBindVertexArray(0);

    // <--- Draw result with post shader --->
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_post.use(FBO::post_buffer.tex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);

    keyboard.swapBuffers();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

