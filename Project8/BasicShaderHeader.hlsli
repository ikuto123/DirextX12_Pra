// BasicShaderHeader.hlsli
struct BasicType
{
    float4 svpos : SV_POSITION;
};

cbuffer cbuff0 : register(b0)
{
    matrix mat; // 変換行列
    float4 baseColor; // ★追加：ベースカラー（四角用・丸用で変える）
};
