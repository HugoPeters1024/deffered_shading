#include "deps.h"

namespace Meshes {

struct Mesh {
  GLuint vao;
  unsigned int vertex_count;
};

Mesh* loadMesh(const char* filename) {
  std::vector<float> vertices, normals, uvs, tangents, bitangents;
  cObj model = cObj(filename);
  model.renderBuffersTangents(vertices, normals, uvs, tangents, bitangents);


  Mesh* mesh = new Mesh();
  mesh->vertex_count = vertices.size();
  
  glGenVertexArrays(1, &mesh->vao);
  glBindVertexArray(mesh->vao);

  GLuint vbo, nbo, uvbo, tbo, btbo;
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &nbo);
  glGenBuffers(1, &uvbo);
  glGenBuffers(1, &tbo);
  glGenBuffers(1, &btbo);

  // Fill vertices
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(D_POS_BUFFER_INDEX);
  glVertexAttribPointer(D_POS_BUFFER_INDEX, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 0));

  // Fill normals
  glBindBuffer(GL_ARRAY_BUFFER, nbo);
  glBufferData(GL_ARRAY_BUFFER, normals.size()*sizeof(float), normals.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(D_NORMAL_BUFFER_INDEX);
  glVertexAttribPointer(D_NORMAL_BUFFER_INDEX, 3, GL_FLOAT, GL_TRUE, 0, (void*)(sizeof(float) * 0)); 

  // Fill uvs
  glBindBuffer(GL_ARRAY_BUFFER, uvbo);
  glBufferData(GL_ARRAY_BUFFER, uvs.size()*sizeof(float), uvs.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(D_UV_BUFFER_INDEX);
  glVertexAttribPointer(D_UV_BUFFER_INDEX, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(float) * 0)); 

  // Fill tangents
  glBindBuffer(GL_ARRAY_BUFFER, tbo);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(D_TANGENT_BUFFER_INDEX);
  glVertexAttribPointer(D_TANGENT_BUFFER_INDEX, 3, GL_FLOAT, GL_TRUE, 0, (void*)(sizeof(float) * 0));

  // Fill bitangents
  glBindBuffer(GL_ARRAY_BUFFER, btbo);
  glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
  glEnableVertexAttribArray(D_BITANGENT_BUFFER_INDEX);
  glVertexAttribPointer(D_BITANGENT_BUFFER_INDEX, 3, GL_FLOAT, GL_TRUE, 0, (void*)(sizeof(float) * 0));

  glBindVertexArray(0);
  return mesh;
}

}

