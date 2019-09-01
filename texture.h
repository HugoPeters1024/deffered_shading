namespace Textures {

typedef GLuint Texture;

Texture loadTexture(const char* filename) {
  int width, height, nrChannels;

  unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
  if (!data) {
    logError("Could not load texture: %s", filename);
    exit(8);
  } else {
    logInfo("Loaded texture %s (%ix%i)", filename, width, height);
  }

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // free image data on host
  stbi_image_free(data);

  return texture;
}

Texture createTextureColor(float rf, float gf, float bf) {
  unsigned char r = (char)(255 * rf);
  unsigned char g = (char)(255 * gf);
  unsigned char b = (char)(255 * bf);
  unsigned char data[3] = {r, g, b};

  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  return texture;
}


}
