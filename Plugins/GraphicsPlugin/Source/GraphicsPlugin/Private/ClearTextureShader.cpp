#include "ClearTextureShader.h"

//                      ShaderClass,	 SourceFile,								EntryPoint, ShaderType {VS, PS, CS...}
IMPLEMENT_GLOBAL_SHADER(FClearTextureCS, "/Plugin/GraphicsPlugin/ClearTexture.usf", "Main",		SF_Compute);