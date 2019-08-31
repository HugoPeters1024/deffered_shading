#include "deps.h"


namespace Shaders {

typedef GLuint Shader;

GLuint loadShaderLiteral(const char* vs, const char* fs) {
    return GenerateProgram(
        CompileShader(GL_VERTEX_SHADER, vs),
        CompileShader(GL_FRAGMENT_SHADER, fs));
}

void initGBufferBinding(const Shader shader) {
    // map uniform textures to slots
    glUniform1i(D_POS_GTEXTURE_INDEX, D_POS_GTEXTURE_INDEX);
    glUniform1i(D_NORMAL_GTEXTURE_INDEX, D_NORMAL_GTEXTURE_INDEX);
    glUniform1i(D_MATERIAL_GTEXTURE_INDEX, D_MATERIAL_GTEXTURE_INDEX);
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


