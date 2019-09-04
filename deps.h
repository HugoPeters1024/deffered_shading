#ifndef H_DEPS
#define H_DEPS

#include <stdio.h>
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES      1
#include <GLFW/glfw3.h>

#include <vector>
#include <math.h>
#include <algorithm>
#include <chrono>
#include <thread>

// SHADER LOCATIONS
// buffers
#define D_POS_BUFFER_INDEX       0
#define D_NORMAL_BUFFER_INDEX    1
#define D_UV_BUFFER_INDEX        2
#define D_TANGENT_BUFFER_INDEX   3
#define D_BITANGENT_BUFFER_INDEX 4

// Uniforms
#define D_CAMERA_UNIFORM_INDEX        0
#define D_MVP_UNIFORM_INDEX           1
#define D_CAMERAPOS_UNIFORM_INDEX     2
#define D_TEXTURE_SCALE_UNIFORM_INDEX 3
#define D_TIME_UNIFORM_INDEX          4

// Textures
#define D_TEXTURE_MATERIAL_INDEX  5
#define D_TEXTURE_NORMALMAP_INDEX 6
#define D_NORMAL_GTEXTURE_INDEX   15
#define D_MATERIAL_GTEXTURE_INDEX 16
#define D_DEPTH_GTEXTURE_INDEX    17

// FRAMEBUFFERS
#define D_FRAMEBUFFER_WIDTH  640
#define D_FRAMEBUFFER_HEIGHT 480

#include "utils/gl_debug.h"
#include "utils/logger.h"
#include "utils/vec.h"
#include "utils/obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "utils/stb_image.h"
#include "utils/shader_utils.h"

#include "keyboard.h"
#include "camera.h"
#include "framebuffer.h"
#include "texture.h"
#include "mesh.h"
#include "shader.h"

#endif
