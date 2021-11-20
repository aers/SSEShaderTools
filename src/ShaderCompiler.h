#pragma once

#include "d3d11.h"

namespace ShaderCompiler
{
	ID3D11PixelShader* CompileAndRegisterPixelShader(const std::wstring a_filePath);
}
