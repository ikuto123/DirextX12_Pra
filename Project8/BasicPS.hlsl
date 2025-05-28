// BasicPS.hlsl
#include "BasicShaderHeader.hlsli"

float4 BasicPS(BasicType input) : SV_TARGET
{
    return baseColor; // ★定数バッファから渡された色で塗る
}
