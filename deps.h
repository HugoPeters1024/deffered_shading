#ifndef H_DEPS
#define H_DEPS

#include <stdio.h>
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h>

#include <vector>

// SHADER LOCATIONS
#define D_POS_BUFFER_INDEX    0
#define D_NORMAL_BUFFER_INDEX 1
#define D_UV_BUFFER_INDEX     2

#define D_CAMERA_UNIFORM_INDEX 0
#define D_MVP_UNIFORM_INDEX    1

#define D_POS_GTEXTURE_INDEX      0
#define D_NORMAL_GTEXTURE_INDEX   1
#define D_MATERIAL_GTEXTURE_INDEX 2

#include "gl_debug.h"
#include "shader_utils.h"
#include "vec.h"
#include "obj_loader.h"
#include "mesh.h"
#include "shader.h"

#endif
