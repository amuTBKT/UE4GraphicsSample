#pragma once

#include "ClearTextureShader.h"

IMPLEMENT_GLOBAL_SHADER(FClearTextureCS, "/Plugin/GraphicsPlugin/ClearTexture.usf", "Main", SF_Compute);