namespace FBO {
  struct g_buffer_t {
    GLuint id, normalTex, materialTex, depthTex;
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void init() {
      printf("Initializing GBuffer\n");
      glGenFramebuffers(1, &id);
      glBindFramebuffer(GL_FRAMEBUFFER, id);

      // Add depth buffering
      glGenTextures(1, &depthTex);
      glBindTexture(GL_TEXTURE_2D, depthTex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0,GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, depthTex,0);

      // Generate and bind 3 textures
      glGenTextures(1, &normalTex);
      glBindTexture(GL_TEXTURE_2D, normalTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, normalTex, 0);

      glGenTextures(1, &materialTex);
      glBindTexture(GL_TEXTURE_2D, materialTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, materialTex, 0);

      // Enable rendering for 3 binding points
      GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
      glDrawBuffers(2, DrawBuffers);

      // Always check that our framebuffer is ok
      auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
          std::cout << "Framebuffer not complete: " << fboStatus <<  " /  " << GL_FRAMEBUFFER_UNSUPPORTED << std::endl;
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        exit(7);
    }
  } g_buffer;

  struct cone_buffer_t {
    GLuint id, tex, depthTex;
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void init() {
      glGenFramebuffers(1, &id);
      glBindFramebuffer(GL_FRAMEBUFFER, id);

      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);

      // Add depth buffering
      glGenTextures(1, &depthTex);
      glBindTexture(GL_TEXTURE_2D, depthTex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0,GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, depthTex,0);

      GLenum DrawBuffers [1] = {GL_COLOR_ATTACHMENT0};
      glDrawBuffers(1, DrawBuffers);
      auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
          std::cout << "Framebuffer not complete: " << fboStatus <<  " /  " << GL_FRAMEBUFFER_UNSUPPORTED << std::endl;
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        exit(7);
    }
  } cone_buffer;

  struct post_buffer_t {
    GLuint id, tex;
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void init() {
      glGenFramebuffers(1, &id);
      glBindFramebuffer(GL_FRAMEBUFFER, id);

      // Generate the only texture
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, D_FRAMEBUFFER_WIDTH, D_FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);

      GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
      glDrawBuffers(1, DrawBuffers);
      // Always check that our framebuffer is ok
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        exit(7);
    }
  } post_buffer;
}
