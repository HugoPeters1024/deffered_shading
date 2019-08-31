#ifndef H_DEPS
#define H_DEPS

#include <stdio.h>
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h>

#include <vector>
#include <math.h>

// SHADER LOCATIONS
#define D_POS_BUFFER_INDEX    0
#define D_NORMAL_BUFFER_INDEX 1
#define D_UV_BUFFER_INDEX     2

#define D_CAMERA_UNIFORM_INDEX 0
#define D_MVP_UNIFORM_INDEX    1
#define D_CAMERAPOS_UNIFORM_INDEX 2

#define D_POS_GTEXTURE_INDEX      15 
#define D_NORMAL_GTEXTURE_INDEX   16
#define D_MATERIAL_GTEXTURE_INDEX 17

#include "gl_debug.h"
#include "shader_utils.h"
#include "vec.h"
#include "obj_loader.h"

#include "keyboard.h"
#include "camera.h"
#include "framebuffer.h"
#include "texture.h"
#include "mesh.h"
#include "shader.h"

#endif
