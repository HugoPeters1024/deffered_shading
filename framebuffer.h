namespace FBO {
  struct g_buffer_t {
    GLuint id, posTex, normalTex, materialTex;
    void bind() const { glBindFramebuffer(GL_FRAMEBUFFER, id); }
    void init() {
      printf("Initializing GBuffer\n");
      glGenFramebuffers(1, &id);
      glBindFramebuffer(GL_FRAMEBUFFER, id);

      // Add depth buffering
      GLuint depthBuf;
      glGenRenderbuffers(1, &depthBuf);
      glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 1920, 1080);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);

      // Generate and bind 3 textures
      glGenTextures(1, &posTex);
      glBindTexture(GL_TEXTURE_2D, posTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, posTex, 0);

      glGenTextures(1, &normalTex);
      glBindTexture(GL_TEXTURE_2D, normalTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, normalTex, 0);

      glGenTextures(1, &materialTex);
      glBindTexture(GL_TEXTURE_2D, materialTex);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1920, 1080, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, materialTex, 0);

      // Enable rendering for 3 binding points
      GLenum DrawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
      glDrawBuffers(3, DrawBuffers);

      // Always check that our framebuffer is ok
      if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        exit(7);
    }
  } g_buffer;
}
