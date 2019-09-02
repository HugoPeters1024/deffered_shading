#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H
#include <stdio.h>
#include <vector>
#include <sstream>
#include <fstream>

#include "vec.h"


struct vertex {
    std::vector<float> v;
    void normalize() {
        float magnitude = 0.0f;
        for (int i = 0; i < v.size(); i++)
            magnitude += pow(v[i], 2.0f);
        magnitude = sqrt(magnitude);
        for (int i = 0; i < v.size(); i++)
            v[i] /= magnitude;
    }
    vertex operator-(vertex v2) {
        vertex v3;
        if (v.size() != v2.v.size()) {
            v3.v.push_back(0.0f);
            v3.v.push_back(0.0f);
            v3.v.push_back(0.0f);
        } else {
            for (int i = 0; i < v.size(); i++)
                v3.v.push_back(v[i] - v2.v[i]);
        }
        return v3;
    }
    vertex cross(vertex v2) {
        vertex v3;
        if (v.size() != 3 || v2.v.size() != 3) {
            v3.v.push_back(0.0f);
            v3.v.push_back(0.0f);
            v3.v.push_back(0.0f);
        } else {
            v3.v.push_back(v[1]*v2.v[2]-v[2]*v2.v[1]);
            v3.v.push_back(v[2]*v2.v[0]-v[0]*v2.v[2]);
            v3.v.push_back(v[0]*v2.v[1]-v[1]*v2.v[0]);
        }
        return v3;
    }
};

struct face {
    std::vector<int> vertex;
    std::vector<int> texture;
    std::vector<int> normal;
};

class cObj {
  private:
    std::vector<vertex> vertices;
    std::vector<vertex> texcoords;
    std::vector<vertex> normals;
    std::vector<vertex> parameters;
    std::vector<face> faces;
  public:
    cObj(std::string filename);
    ~cObj();

  void renderBuffers(std::vector<float> &v_buf, std::vector<float> &n_buf, std::vector<float> &uv_buf) const;
  void renderBuffersTangents(std::vector<float> &v_buf, std::vector<float> &n_buf, std::vector<float> &uv_buf, std::vector<float> &t_buf, std::vector<float> &bt_buf) const;
};

cObj::cObj(std::string filename) {
    std::ifstream ifs(filename.c_str(), std::ifstream::in);
    std::string line, key;
    while (ifs.good() && !ifs.eof() && std::getline(ifs, line)) {
        key = "";
        std::stringstream stringstream(line);
        stringstream >> key >> std::ws;


        if (key == "v") { // vertex
            vertex v; float x;
            while (!stringstream.eof()) {
                stringstream >> x >> std::ws;
                v.v.push_back(x);
            }
            vertices.push_back(v);
        } else if (key == "vp") { // parameter
            vertex v; float x;
            while (!stringstream.eof()) {
                stringstream >> x >> std::ws;
                v.v.push_back(x);
            }
            parameters.push_back(v);
        } else if (key == "vt") { // texture coordinate
            vertex v; float x;
            while (!stringstream.eof()) {
                stringstream >> x >> std::ws;
                v.v.push_back(x);
            }
            texcoords.push_back(v);
        } else if (key == "vn") { // normal
            vertex v; float x;
            while (!stringstream.eof()) {
                stringstream >> x >> std::ws;
                v.v.push_back(x);
            }
            v.normalize();
            normals.push_back(v);
        } else if (key == "f") { // face
            face f; int v, t, n;
            while (!stringstream.eof()) {
                stringstream >> v >> std::ws;
                f.vertex.push_back(v-1);
                if (stringstream.peek() == '/') {
                    stringstream.get();
                    if (stringstream.peek() == '/') {
                        stringstream.get();
                        stringstream >> n >> std::ws;
                        f.normal.push_back(n-1);
                    } else {
                        stringstream >> t >> std::ws;
                        f.texture.push_back(t-1);
                        if (stringstream.peek() == '/') {
                            stringstream.get();
                            stringstream >> n >> std::ws;
                            f.normal.push_back(n-1);
                        }
                    }
                }
            }
            faces.push_back(f);
        } else {

        }
    }
    ifs.close();
    std::cout << "               Name: " << filename << std::endl;
    std::cout << "           Vertices: " << vertices.size() << std::endl;
    std::cout << "         Parameters: " << parameters.size() << std::endl;
    std::cout << "Texture Coordinates: " << texcoords.size() << std::endl;
    std::cout << "            Normals: " << normals.size() << std::endl;
    std::cout << "              Faces: " << faces.size() << std::endl << std::endl;
}

void cObj::renderBuffers(std::vector<float> &v_buf, std::vector<float> &n_buf, std::vector<float> &uv_buf) const
{
  for(face f : faces)
  {
    if (f.vertex.size() == 3 && f.normal.size() == 3)
    {
      for(int i = 0; i<3; i++)
      {
        int vi = f.vertex[i];
        int ni = f.normal[i];
        int ti = f.texture[i];
        v_buf.push_back(vertices[vi].v[0]);
        v_buf.push_back(vertices[vi].v[1]);
        v_buf.push_back(vertices[vi].v[2]);

        n_buf.push_back(normals[ni].v[0]);
        n_buf.push_back(normals[ni].v[1]);
        n_buf.push_back(normals[ni].v[2]);

        if (texcoords.size() > 0) { 
          uv_buf.push_back(texcoords[ti].v[0]);
          uv_buf.push_back(texcoords[ti].v[1]);
        }
      }
    }
    else if (f.vertex.size() == 4 && f.normal.size() == 4)
    {
        // Extract the 4 vertices
        vertex v1 = vertices[f.vertex[0]];
        vertex v2 = vertices[f.vertex[1]];
        vertex v3 = vertices[f.vertex[2]];
        vertex v4 = vertices[f.vertex[3]];

        // Extract the 4 normals
        vertex n1 = normals[f.normal[0]];
        vertex n2 = normals[f.normal[1]];
        vertex n3 = normals[f.normal[2]];
        vertex n4 = normals[f.normal[3]];

        vertex t1, t2, t3, t4;
        if (texcoords.size() > 0) {
          // Extract the 4 texcoords
          t1 = texcoords[f.texture[0]];
          t2 = texcoords[f.texture[1]];
          t3 = texcoords[f.texture[2]];
          t4 = texcoords[f.texture[3]];
        }


        // Triangle 1
        v_buf.push_back(v1.v[0]);
        v_buf.push_back(v1.v[1]);
        v_buf.push_back(v1.v[2]);

        v_buf.push_back(v2.v[0]);
        v_buf.push_back(v2.v[1]);
        v_buf.push_back(v2.v[2]);

        v_buf.push_back(v3.v[0]);
        v_buf.push_back(v3.v[1]);
        v_buf.push_back(v3.v[2]);

        // Triangle 2
        v_buf.push_back(v1.v[0]);
        v_buf.push_back(v1.v[1]);
        v_buf.push_back(v1.v[2]);

        v_buf.push_back(v3.v[0]);
        v_buf.push_back(v3.v[1]);
        v_buf.push_back(v3.v[2]);

        v_buf.push_back(v4.v[0]);
        v_buf.push_back(v4.v[1]);
        v_buf.push_back(v4.v[2]);

        // Triangle 1 Normals
        n_buf.push_back(n1.v[0]);
        n_buf.push_back(n1.v[1]);
        n_buf.push_back(n1.v[2]);

        n_buf.push_back(n2.v[0]);
        n_buf.push_back(n2.v[1]);
        n_buf.push_back(n2.v[2]);

        n_buf.push_back(n3.v[0]);
        n_buf.push_back(n3.v[1]);
        n_buf.push_back(n3.v[2]);

        // Triangle 2 Normals
        n_buf.push_back(n1.v[0]);
        n_buf.push_back(n1.v[1]);
        n_buf.push_back(n1.v[2]);

        n_buf.push_back(n3.v[0]);
        n_buf.push_back(n3.v[1]);
        n_buf.push_back(n3.v[2]);

        n_buf.push_back(n4.v[0]);
        n_buf.push_back(n4.v[1]);
        n_buf.push_back(n4.v[2]);
        
        if (texcoords.size() > 0) {
          // Triangle 1 textures
          uv_buf.push_back(t1.v[0]);
          uv_buf.push_back(t1.v[1]);

          uv_buf.push_back(t2.v[0]);
          uv_buf.push_back(t2.v[1]);

          uv_buf.push_back(t3.v[0]);
          uv_buf.push_back(t3.v[1]);

          // Triangle 2 textures
          uv_buf.push_back(t1.v[0]);
          uv_buf.push_back(t1.v[1]);

          uv_buf.push_back(t3.v[0]);
          uv_buf.push_back(t3.v[1]);

          uv_buf.push_back(t4.v[0]);
          uv_buf.push_back(t4.v[1]);
        }
        else {
          unsigned int vsize = v_buf.size();
          Vector3 v1 = Vector3(v_buf[vsize - 8], v_buf[vsize - 7], v_buf[vsize - 6]);
          Vector3 v2 = Vector3(v_buf[vsize - 5], v_buf[vsize - 4], v_buf[vsize - 3]);
          Vector3 v3 = Vector3(v_buf[vsize - 2], v_buf[vsize - 1], v_buf[vsize - 0]);

          Vector3 l1 = (v2 - v1);
          Vector3 l2 = (v3 - v1);

          Vector3 p = (v2 + v3) / 2;
          Vector3 o = 2 * p;


          // Generate uv's
          // T1
          uv_buf.push_back(0);
          uv_buf.push_back(0);

          uv_buf.push_back(0);
          uv_buf.push_back(0);

          uv_buf.push_back(0);
          uv_buf.push_back(0);

          // T2
          uv_buf.push_back(0);
          uv_buf.push_back(0);

          uv_buf.push_back(0);
          uv_buf.push_back(0);

          uv_buf.push_back(0);
          uv_buf.push_back(0);
        }
    }
    else
    {
      printf("Cannot serialize a model that has %lu vertices and %lu normals per face\n", f.vertex.size(), f.normal.size());
      exit(4);
    }
  }
  printf("Expanded %zu faces to %zu coordinates\n", faces.size(), v_buf.size());
}

void cObj::renderBuffersTangents(std::vector<float> &v_buf, std::vector<float> &n_buf, std::vector<float> &uv_buf, std::vector<float> &t_buf, std::vector<float> &bt_buf) const
{
  renderBuffers(v_buf, n_buf, uv_buf);
  if (uv_buf.size() == 0) {
    logWarning("Model does not contain uv coordinates, nulling tangent data");
    return;
  }
  for(int i=0, ui=0; i<v_buf.size(); i+=9, ui+=6)
  {
     Vector3 v1 = Vector3(v_buf[i+0], v_buf[i+1], v_buf[i+2]);
     Vector3 v2 = Vector3(v_buf[i+3], v_buf[i+4], v_buf[i+5]);
     Vector3 v3 = Vector3(v_buf[i+6], v_buf[i+7], v_buf[i+8]);

     Vector2 uv1 = Vector2(uv_buf[ui+0], uv_buf[ui+1]);
     Vector2 uv2 = Vector2(uv_buf[ui+2], uv_buf[ui+3]);
     Vector2 uv3 = Vector2(uv_buf[ui+4], uv_buf[ui+5]);

     Vector3 vedge1 = v2 - v1;
     Vector3 vedge2 = v3 - v1;
     Vector2 uedge1 = uv2 - uv1;
     Vector2 uedge2 = uv3 - uv1;

     float r = 1.0f / (uedge1.x * uedge2.y - uedge1.y * uedge2.x);
     Vector3 tangent = ((vedge1 * uedge2.y - vedge2 * uedge1.y) * r).normalize();
     Vector3 bitangent = ((vedge2 * uedge1.x - vedge1 * uedge2.x) * r).normalize();

     for(int q=0; q<3; q++) {
       t_buf.push_back(tangent.x);
       t_buf.push_back(tangent.y);
       t_buf.push_back(tangent.z);

       bt_buf.push_back(bitangent.x);
       bt_buf.push_back(bitangent.y);
       bt_buf.push_back(bitangent.z);
     }
  }
}

cObj::~cObj() { }

#endif
