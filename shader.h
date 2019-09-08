#include "deps.h"
namespace Shaders {

using namespace Textures;

static const char* vs_src = R"(
#version 450
layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vUv;
layout(location=3) in vec3 vTangent;
layout(location=4) in vec3 vBitangent;

out vec3 position;
out vec3 normal;
out vec2 uv;
out vec3 tangent;
out vec3 bitangent;
out flat mat3 TBN;
out flat int usenormalmap;

layout(location = 0) uniform mat4 uCamera;
layout(location = 1) uniform mat4 uMvp;

void main() {
  vec4 worldPos = uMvp * vec4(vPos, 1); 
  gl_Position = uCamera * worldPos;
  position = worldPos.xyz;
  normal = normalize(uMvp * vec4(vNormal, 0)).xyz;
  tangent = normalize(uMvp * vec4(vTangent, 0)).xyz;
  bitangent = normalize(uMvp * vec4(vBitangent, 0)).xyz;
  uv = vUv;
  if (uv.x != 0 && uv.y != 0) {
    TBN = inverse(mat3(tangent, bitangent, normal));
    usenormalmap = 1;
  }
  else {
    usenormalmap = 0;
  }
}
)";

static const char* fs_src = R"(
#version 450

in vec3 position;
in vec3 normal;
in vec2 uv;
in vec3 tangent;
in vec3 bitangent;
in flat mat3 TBN;
in flat int usenormalmap;

out vec3 c_normal;
out vec3 c_material;

layout(location = 3) uniform float texture_scale;
layout(location = 5) uniform sampler2D material;
layout(location = 6) uniform sampler2D normalmap;

void main() {
  c_material = texture(material, uv * texture_scale).xyz; 
  if (usenormalmap == 1) 
  {
    // Read normal and restore the range
    vec3 mn = texture(normalmap, uv * texture_scale).xyz * 2 - 1;
    mn = vec3(-mn.x, mn.y, mn.z);
    // Transform the normal to worldspace
    c_normal = normalize(mn * TBN); 
  }
  else 
  {
    c_normal = normal;
  }

  // Compress the normal
  c_normal = c_normal / 2 + 0.5;
}
)";

static const char* quad_vs_src = R"( 
#version 450
in vec3 vPos;

out vec2 uv;

void main() {
 gl_Position = vec4(vPos, 1);
 uv = vPos.xy/2 + 0.5;
}
)";

static const char* quad_fs_src = R"(
#version 450

in vec2 uv;

out vec3 color;
uniform sampler2D tex;

void main() {
  color = texture(tex, uv).xyz;
}
)";

static const char* empty_fs_src = R"(
#version 450

out vec3 color;

void main() {
  color = vec3(1, 0, 1);
}
)";

static const char* post_fs_src = R"(
#version 450

in vec2 uv;
out vec3 color;

layout(location = 17) uniform sampler2D scene;
layout(location = 18) uniform sampler2D cones;
layout(location = 4) uniform float time;

void main() {
 vec3 conemap = texture(cones, uv).xyz;
 float b = length(conemap);
 for(int s=0; s<9; s++) {
     float f = (s / 9.0f) * 6.2832f;
     vec2 offset = vec2(sin(f), cos(f)) / (100.0f + 10000 * pow(b, 100));
     conemap += texture(cones, uv + offset).xyz;
 }
 color = texture(scene, uv).xyz + conemap / 10.0f;
}

)";

static const char* defer_fs_src = R"(
#version 450

in vec2 uv;

layout(location = 0)  uniform mat4      uCamera;
layout(location = 2)  uniform vec3      uCamPos;
layout(location = 15) uniform sampler2D t_normal;
layout(location = 16) uniform sampler2D t_material;
layout(location = 17) uniform sampler2D t_depth;

out vec3 color;

layout (std140) uniform shader_data
{
  vec4 light_pos[32];
  vec4 light_col[32];
  vec4 light_dir[32];
};

// this is supposed to get the world position from the depth buffer
vec3 WorldPosFromDepth(float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverse(uCamera) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    vec4 worldSpacePosition = viewSpacePosition;

    return worldSpacePosition.xyz;
}

void main() {
  vec3 normal = normalize(texture(t_normal, uv).xyz * 2 - 1);
  vec3 material = texture(t_material, uv).xyz;
  float depth = texture(t_depth, uv).x;
  vec3 pos = WorldPosFromDepth(depth);
  vec3 E = normalize(uCamPos - pos);

  // ambient
  color = material * 0.2;
  for(int i=0; i<32; i++) {
    vec3 lVec = light_pos[i].xyz - pos;
    float dist2 = dot(lVec, lVec);
    vec3 lDir = lVec / sqrt(dist2);
    vec3 lNormal = light_dir[i].xyz;
    float cone = light_dir[i].w;

    float corr = 1;
    float cone_angle = max(dot(lDir, -lNormal), 0);
    if (cone_angle < cone)
      corr = max(1 - 16 * abs(cone_angle-cone), 0);

    float falloff = 1 / dist2;
    float diffuse = max(dot(lDir, normal), 0);

    vec3 R = reflect(-lDir, normal);
    float specular = pow(max(dot(E, R), 0), 40);
    color += (specular + diffuse) * material * light_col[i].xyz * falloff * corr;
  }

  float mist = pow(depth, 1000);
  color = mist * vec3(0.25) + (1-mist) * color;
}
)";

static const char* empty_vs_src = R"(
#version 450

void main() { }
)";

static const char* cone_gs_src = R"(
#version 450

#define PREC 20

layout(points) in;
layout(triangle_strip, max_vertices=(PREC*3)) out;

layout(location=0) uniform mat4 camera;
layout(location=1) uniform mat4 mvp;
layout(location=2) uniform vec3 camPos;
layout(location=3) uniform vec4 cone_data;

struct vData {
  float dist2;
};

out vData vertex;

float getAngle(int p) {
  return (p / float(PREC)) * (2*3.14159265358979323846);
}

mat4 rotationX( in float angle ) {
	return mat4(	1.0,		0,			0,			0,
			 		0, 	cos(angle),	-sin(angle),		0,
					0, 	sin(angle),	 cos(angle),		0,
					0, 			0,			  0, 		1);
}

mat4 rotationY( in float angle ) {
	return mat4(	cos(angle),		0,		sin(angle),	0,
			 				0,		1.0,			 0,	0,
					-sin(angle),	0,		cos(angle),	0,
							0, 		0,				0,	1);
}

mat4 rotationZ( in float angle ) {
	return mat4(	cos(angle),		-sin(angle),	0,	0,
			 		sin(angle),		cos(angle),		0,	0,
							0,				0,		1,	0,
							0,				0,		0,	1);
}

void main() {
  mat4 t = camera * mvp;
  vec3 origin = vec3(0, 0, 0);
  vec4 p0 = t * vec4(origin, 1);
  float angle = acos(cone_data.w);
  vec3 on_circle = (rotationX(angle) * vec4(0, 0, 1, 1)).xyz; 
  vec3 L;

  for(int p=0; p<PREC; p++) {
    float a1 = getAngle(p);
    float a2 = getAngle(p+1);
    vec4 p1 = t * rotationZ(a1) * vec4(on_circle, 1);
    vec4 p2 = t * rotationZ(a2) * vec4(on_circle, 1);
    gl_Position = p0; 
    vertex.dist2 = 0;
    EmitVertex();
    gl_Position = p1;
    L = gl_Position.xyz - p0.xyz;
    vertex.dist2 = dot(L, L);
    EmitVertex();
    gl_Position = p2;
    L = gl_Position.xyz - p0.xyz;
    vertex.dist2 = dot(L, L);
    EmitVertex();
    EndPrimitive();
  }
}
)";

static const char* plane_vs_src = R"(
#version 450

layout(location = 0) in vec3 vPos;


void main() {
  gl_Position = vec4(vPos, 1);
}

)";


static const char* plane_gs_src = R"(
#version 450

layout(points) in;
layout(triangle_strip, max_vertices=6) out;

layout(location = 0) uniform mat4 camera;
layout(location = 1) uniform mat4 mvp;

struct vData {
  vec3 normal;
  vec3 pos;
  float h;
};

out vData vertex; 

float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}
float noise(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);
    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);
    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);
    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));
    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);
    return o4.y * d.y + o4.x * (1.0 - d.y);
}


void main() {
  mat4 t =  camera * mvp;
  vec3 pos[4];
  pos[0] = gl_in[0].gl_Position.xyz + vec3(0, 0, 0);
  pos[1] = gl_in[0].gl_Position.xyz + vec3(1, 0, 0);
  pos[2] = gl_in[0].gl_Position.xyz + vec3(0, 0, 1);
  pos[3] = gl_in[0].gl_Position.xyz + vec3(1, 0, 1);
  float h[4];
  h[0] = noise(pos[0] / 4.0);
  h[1] = noise(pos[1] / 4.0);
  h[2] = noise(pos[2] / 4.0);
  h[3] = noise(pos[3] / 4.0);
  pos[0] = pos[0] + vec3(0, h[0] * 10, 0);
  pos[1] = pos[1] + vec3(0, h[1] * 10, 0);
  pos[2] = pos[2] + vec3(0, h[2] * 10, 0);
  pos[3] = pos[3] + vec3(0, h[3] * 10, 0);
  vec3 normal[2];
  normal[0] = -normalize((mvp * vec4(cross(normalize(pos[0] - pos[1]), normalize(pos[1] - pos[2])), 0))).xyz;
  normal[1] = normalize((mvp * vec4(cross(normalize(pos[1] - pos[2]), normalize(pos[2] - pos[3])), 0))).xyz;

  vertex.pos = pos[0]; 
  vertex.h = h[0];
  vertex.normal = normal[0];
  gl_Position = t * vec4(pos[0], 1);
  EmitVertex();

  vertex.pos = pos[1]; 
  vertex.h = h[1];
  vertex.normal = normal[0];
  gl_Position = t * vec4(pos[1], 1);
  EmitVertex();

  vertex.pos = pos[2]; 
  vertex.h = h[2];
  vertex.normal = normal[0];
  gl_Position = t * vec4(pos[2], 1);
  EmitVertex();

  EndPrimitive();

  vertex.pos = pos[1]; 
  vertex.h = h[1];
  vertex.normal = normal[1];
  gl_Position = t * vec4(pos[1], 1);
  EmitVertex();

  vertex.pos = pos[2]; 
  vertex.h = h[2];
  vertex.normal = normal[1];
  gl_Position = t * vec4(pos[2], 1);
  EmitVertex();

  vertex.pos = pos[3]; 
  vertex.h = h[3];
  vertex.normal = normal[1];
  gl_Position = t * vec4(pos[3], 1);
  EmitVertex();

  EndPrimitive();
}
)";

static const char* plane_fs_src = R"(
#version 450

layout(location = 3) uniform float texture_scale;

struct vData {
  vec3 normal;
  vec3 pos;
  float h;
};

layout(location = 5) uniform sampler2D t_water;
layout(location = 7) uniform sampler2D t_grass;
layout(location = 8) uniform sampler2D t_stone;

in vData vertex;
out vec3 c_normal;
out vec3 c_material;

void main() {
  vec3 water = texture(t_water, vertex.pos.xz * texture_scale).xyz;
  vec3 grass = texture(t_grass, vertex.pos.xz * texture_scale).xyz;
  vec3 stone = texture(t_stone, vertex.pos.xz * texture_scale).xyz;
  float c = -0.0;
  float a1 = pow(max(cos((vertex.h-c) * 3.14), 0), 4) + 0.1;
  float a2 = pow(max(cos((vertex.h-c-0.5) * 3.14), 0), 4) + 0.1;
  float a3 = pow(max(cos((vertex.h-c-1) * 3.14), 0), 4) + 0.1;
  c_material = a1 * water + a2 * grass + a3 * stone;
  c_normal   = vertex.normal;
}
)";


static const char* cone_fs_src = R"(
#version 450

layout(location = 17) uniform sampler2D depthBuf;
layout(location = 4)  uniform vec3 light_col;
layout(location = 0)  out vec3 color;

struct vData {
  float dist2;
};

in vData vertex;

void main() {
  vec2 uv = vec2(gl_FragCoord.x/640.0f, gl_FragCoord.y/480.0f); 
  float old_depth = texture(depthBuf, uv).r;
  float new_depth = gl_FragCoord.z;
  if (old_depth > new_depth)
    color = light_col / (pow(vertex.dist2, 0.45) * 300);
  else
    color = vec3(0);
}
)";

static inline GLuint loadShaderLiteral(const char* vs, const char* fs) {
    return GenerateProgram(
        CompileShader(GL_VERTEX_SHADER, vs),
        CompileShader(GL_FRAGMENT_SHADER, fs));
}

static inline GLuint loadShaderLiteral(const char* vs, const char* gs, const char* fs) {
    return GenerateProgram(
        CompileShader(GL_VERTEX_SHADER, vs),
        CompileShader(GL_GEOMETRY_SHADER, gs),
        CompileShader(GL_FRAGMENT_SHADER, fs));
}

static GLuint lights_buffer;

struct lights_t {
  Vector4 pos[32];
  Vector4 col[32];
  Vector4 dir[32]; // xyz for direction, w for cone size (0 normal light)
};

struct sh_quad_t {
  // Simple shader that renders a single texture to a quad
  GLuint program_id;
  void use(const Texture &tex) const  {
    glUseProgram(program_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
  }
} sh_quad;

struct sh_main_t {
  // Main shader that renders to the g buffers
  GLuint program_id;
  void setCamera(const Matrix4 &camera) const {
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
  }
  void setMvp(const Matrix4 &mvp) const {
    mat4x4 m_mvp;
    mvp.unpack(m_mvp);
    glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_mvp);
  }
  void setTextureScale(float s) const {
    glUniform1f(D_TEXTURE_SCALE_UNIFORM_INDEX, s);
  }
  void use(const Matrix4 &camera) const {
    glUseProgram(program_id);
    setCamera(camera);
  }
} sh_main;

struct sh_cone_t {
  GLuint program_id;
  void setCamera(const Matrix4 &camera, const Vector3 &cam_pos) const {
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
    glUniform3f(D_CAMERAPOS_UNIFORM_INDEX, cam_pos.x, cam_pos.y, cam_pos.z);
  }
  void setMvp(const Matrix4 &mvp) const {
    mat4x4 m_mvp;
    mvp.unpack(m_mvp);
    glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_mvp);
  }
  void setCone(const Vector4 &dir, const Vector4 &color) {
    glUniform4f(3, dir.x, dir.y, dir.z, dir.w);
    glUniform3f(4, color.x, color.y, color.z);
  }
  void use(const Matrix4 &camera, const Vector3 &cam_pos, const Texture &depth) const {
    glUseProgram(program_id);
    setCamera(camera, cam_pos);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth);
  }
} sh_cone;

struct sh_plane_t {
  GLuint program_id;
  void setCamera(const Matrix4 &camera, const Vector3 &cam_pos) const {
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
    glUniform3f(D_CAMERAPOS_UNIFORM_INDEX, cam_pos.x, cam_pos.y, cam_pos.z);
  }
  void setMvp(const Matrix4 &mvp) const {
    mat4x4 m_mvp;
    mvp.unpack(m_mvp);
    glUniformMatrix4fv(D_MVP_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_mvp);
  }
  void setTextureScale(float s) const {
    glUniform1f(D_TEXTURE_SCALE_UNIFORM_INDEX, s);
  }
  void use(const Matrix4 &camera,
      const Vector3 &cam_pos,
      const Texture &water,
      const Texture &grass,
      const Texture &stone) const {
    glUseProgram(program_id);
    setCamera(camera, cam_pos);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, water);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, grass);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, stone);
    glActiveTexture(GL_TEXTURE0);
  }
} sh_plane;

struct sh_combinator_t {
  // Combination shader that combines to the g buffers to a quad
  GLuint program_id;
  void use(const lights_t &lights,
      const Matrix4 &camera,
      const Vector3 &cam_pos,
      Texture g_norm,
      Texture g_mat,
      Texture g_depth) const
  {
    glUseProgram(program_id);
    // Update te light suite
    glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), &lights);

    // Populate g buffer slots
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_norm);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_mat);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, g_depth);

    // Populate camera transformation for restoring the fragment position from depth buffer
    mat4x4 m_camera;
    camera.unpack(m_camera);
    glUniformMatrix4fv(D_CAMERA_UNIFORM_INDEX, 1, GL_FALSE, (const GLfloat*)m_camera);
    
    // Populate camera pos for specular lighting
    glUniform3f(D_CAMERAPOS_UNIFORM_INDEX, cam_pos.x, cam_pos.y, cam_pos.z);
  }
} sh_combinator;

struct sh_post_t {
  GLuint program_id;
  void use(const Texture &scene, const Texture &cones, float time) const {
    glUseProgram(program_id);
    glUniform1f(D_TIME_UNIFORM_INDEX, time);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, scene);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cones);
    glActiveTexture(GL_TEXTURE0);
  }
} sh_post;

void init() {
  printf("Shaders are being initalized\n");

  // QUAD SHADER
  sh_quad.program_id = loadShaderLiteral(quad_vs_src, quad_fs_src);
  // MAIN SHADER
  logInfo("Compiling main shader");
  sh_main.program_id = loadShaderLiteral(vs_src, fs_src);
  glUseProgram(sh_main.program_id);
  glUniform1i(D_TEXTURE_MATERIAL_INDEX, 0);
  glUniform1i(D_TEXTURE_NORMALMAP_INDEX, 1);
  sh_main.setTextureScale(1);
  logInfo("Main shader compiled succesfully");

  // CONE SHADER
  logInfo("Compiling cone shader");
  sh_cone.program_id = loadShaderLiteral(empty_vs_src, cone_gs_src, cone_fs_src);
  glUseProgram(sh_cone.program_id);
  glUniform1i(D_DEPTH_GTEXTURE_INDEX, 0);
  logInfo("Compiling cone shader completed (id: %i)", sh_cone.program_id);

  // PLANE SHADER
  logInfo("Compiling plane shader");
  sh_plane.program_id = loadShaderLiteral(plane_vs_src, plane_gs_src, plane_fs_src);
  glUseProgram(sh_plane.program_id);
  glUniform1i(D_TEXTURE_MATERIAL_INDEX,  0);
  glUniform1i(D_TEXTURE_MATERIAL2_INDEX, 1);
  glUniform1i(D_TEXTURE_MATERIAL3_INDEX, 2);
  logInfo("Compiling plane shader completed (id: %i)", sh_plane.program_id);


  // POST SHADER
  logInfo("Compiling post processing shader");
  sh_post.program_id = loadShaderLiteral(quad_vs_src, post_fs_src);
  glUseProgram(sh_post.program_id);
  glUniform1i(17, 0);
  glUniform1i(18, 1);
  logInfo("post processing shader compiled succesfully");


  // COMBINATOR
  logInfo("Compiling combination shader");
  sh_combinator.program_id = loadShaderLiteral(quad_vs_src, defer_fs_src); 
  glUseProgram(sh_combinator.program_id);
  logInfo("Combination shader compiled succesfully");

  // g buffer bindings
  glUniform1i(D_NORMAL_GTEXTURE_INDEX,         0);
  glUniform1i(D_MATERIAL_GTEXTURE_INDEX,       1);
  glUniform1i(D_DEPTH_GTEXTURE_INDEX,          2);

  // lights
  glGenBuffers(1, &lights_buffer);
  glBindBuffer(GL_UNIFORM_BUFFER, lights_buffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(lights_t), NULL, GL_DYNAMIC_DRAW);

  GLuint u_shader_data = glGetUniformBlockIndex(sh_combinator.program_id, "shader_data"); 
  glBindBufferBase(GL_UNIFORM_BUFFER, u_shader_data, lights_buffer);
  // END COMBINATOR
}

}


