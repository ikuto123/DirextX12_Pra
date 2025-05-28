// BasicVS.hlsl
#include "BasicShaderHeader.hlsli"

BasicType BasicVS(float3 pos : POSITION, float2 uv : TEXCOORD)
{
    BasicType output;
    output.svpos = mul(mat, float4(pos, 1.0f));
    return output;
}
