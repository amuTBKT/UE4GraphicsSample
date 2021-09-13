#include "ProceduralMesh/MarchingCubeShaders/MarchingCubeShaders.h"

IMPLEMENT_GLOBAL_SHADER(FResetIndirectArgsCS, "/Plugin/GraphicsPlugin/MarchingCubes/ResetIndirectDraw.usf", "Main", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FGenerateTrianglesCS, "/Plugin/GraphicsPlugin/MarchingCubes/GenerateTriangles.usf", "Main", SF_Compute);