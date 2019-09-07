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

  GLFWwindow* window = glfwCreateWindow(D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, "Yeah", NULL, NULL);
  if (!window) return 3;
  glfwMakeContextCurrent(window);
  glEnable(GL_DEPTH_TEST);

  // Print info
  const GLubyte* vendor = glGetString(GL_VENDOR);
  logDebug("Video card:\t\t%s", vendor);
  const GLubyte* renderer = glGetString(GL_RENDERER);
  logDebug("Renderer:\t\t%s", renderer);
  const GLubyte* version = glGetString(GL_VERSION);
  logDebug("OpenGL version:\t%s", version);

  FBO::g_buffer.init();
  FBO::cone_buffer.init();
  FBO::post_buffer.init();

  Camera::Camera camera = Camera::Camera(1.25);
  camera.setPosition(Vector3(0, 10, 20));

  Keyboards::Keyboard keyboard = Keyboards::Keyboard(window);

  Shaders::init();
  Shaders::lights_t lights;

  Textures::init();

  auto quad = Meshes::loadMesh("quad.obj");
  auto mesh = Meshes::loadMesh("player.obj");
  auto cube = Meshes::loadMesh("cube.obj");
  auto floor = Meshes::loadMesh("floor.obj");
  float cone_data[] = {
    0, 0, 0,
  };
  auto cone = Meshes::loadMeshPoints(3, cone_data);

  float* plane_data = (float*)malloc(25 * 25 * 3 * sizeof(float));
  for(int y=0, q=0; y<25; y++) {
    for(int x=0; x<25; x++) {
      plane_data[q++] = ((float)x);
      plane_data[q++] = 0;
      plane_data[q++] = ((float)y);
    }
  }
  auto plane = Meshes::loadMeshPoints(25*25*3, plane_data);
  delete plane_data;

  auto tx_white = Textures::createTextureColor(1, 1, 1);
  auto tx_brick = Textures::loadTexture("textures/wall.jpg");
  auto tx_brick_norm = Textures::loadTexture("textures/wall_norm.jpg");
  auto tx_water = Textures::loadTexture("textures/water.jpg");
  auto tx_grass = Textures::loadTexture("textures/grass.jpg");
  auto tx_stone = Textures::loadTexture("textures/stone.jpg");

  int int_Time = 0;
  float time = 0;

  while(!glfwWindowShouldClose(window))
  {
    int_Time += 1;
    time = glfwGetTime() / 2;

    for(int i=0; i<16; i+=1) {
      float v = (float)i / 16.0f * 6.28;
      float x = sin(v + time);
      float z = cos(v + time);
      float r = 50 + 35 * sin(time); 
      lights.pos[i] = Vector4(r * x, 45 + 20 * cos(4 * v + 4 * time), r * z, 0);
      lights.col[i] = Vector4(0.5 * x + 0.5, 0.5*z +0.5, 0.5, 1) * 1800;
      lights.dir[i] = Vector4((Vector3(0, 20, 0)-lights.pos[i].xyz()).normalized(), 0.999 - 0.001 * sin(v+glfwGetTime()));
    }

    lights.pos[16] = Vector4(0, 70, 0, 0);
    lights.col[16] = Vector4(1) * 500;
    lights.dir[16] = Vector4(0, -1, 0, 0.994);

    for(int i=17; i<32; i++) {
      lights.pos[i] = Vector4(1000000000);
      lights.col[i] = Vector4(0);
      lights.dir[i] = Vector4(0);
    }


    int w, h;
    Matrix4 mvp;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);

    camera.update(w/h, &keyboard);


    FBO::g_buffer.bind();
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    Shaders::sh_main.use(camera.getMatrix());
    Textures::disableNormalMap();
    Textures::setTexture(tx_white);
    mvp = Matrix4::Identity();
    glBindVertexArray(mesh->vao);
    Shaders::sh_main.setMvp(mvp);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);


    glBindVertexArray(cube->vao);
    for(int i=0; i<32; i++) {
      Matrix4 cube_mvp = Matrix4::FromTranslation(lights.pos[i].xyz());
      Shaders::sh_main.setMvp(cube_mvp);
      glDrawArrays(GL_TRIANGLES, 0, cube->vertex_count);
    }

    Textures::setTexture(tx_brick);
    Shaders::sh_main.setTextureScale(5);

    /*
    Textures::setNormalMap(tx_brick_norm);
    Matrix4 floor_mvp = Matrix4::FromScale(150, 150, 150);
    Shaders::sh_main.setMvp(floor_mvp);
    glBindVertexArray(floor->vao);
    glDrawArrays(GL_TRIANGLES, 0, floor->vertex_count);
    Textures::disableNormalMap();
    */



    // <--- Draw combined to post buffer ---->
    FBO::post_buffer.bind();
    glViewport(0, 0, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_combinator.use(
        lights,
        camera.getMatrix(),
        camera.getPosition(),
        FBO::g_buffer.normalTex,
        FBO::g_buffer.materialTex,
        FBO::g_buffer.depthTex);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);
    glBindVertexArray(0);


    // Blend the cones over the over the scenes
    FBO::cone_buffer.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mvp = Matrix4::FromAxisRotations(0, 0, 0);
    Shaders::sh_cone.use(camera.getMatrix(), camera.getPosition(), FBO::g_buffer.depthTex);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glBindVertexArray(cone->vao);
    for(int i=0; i<17; i++) {
      mvp = Matrix4::FromTranslation(lights.pos[i].xyz()) * 
        Matrix4::FromNormal(lights.dir[i].xyz()) * 
        Matrix4::FromScale(300);
      Shaders::sh_cone.setMvp(mvp);
      Shaders::sh_cone.setCone(lights.dir[i], lights.col[i]);
      glDrawArrays(GL_POINTS, 0, cone->vertex_count);
    }
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);


    // <--- Draw result with post shader --->
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shaders::sh_post.use(
        FBO::post_buffer.tex,
        FBO::cone_buffer.tex,
        time);
    glBindVertexArray(quad->vao);
    glDrawArrays(GL_TRIANGLES, 0, quad->vertex_count);

    // test
    glClear(GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    Shaders::sh_plane.use(
        camera.getMatrix(),
        camera.getPosition(),
        tx_water,
        tx_grass,
        tx_stone);
    Shaders::sh_plane.setTextureScale(0.05);
    Shaders::sh_plane.setMvp(
        Matrix4::FromScale(10) * 
        Matrix4::FromTranslation(-12.5, -10, -12.5));
    glBindVertexArray(plane->vao);
    glDrawArrays(GL_POINTS, 0, plane->vertex_count);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    keyboard.swapBuffers();
    glfwSwapInterval(1);
    glfwSwapBuffers(window);
    glfwPollEvents();
//    std::this_thread::sleep_for(std::chrono::milliseconds(1000/80));
  }
}

