// BasicShaderHeader.hlsli
struct BasicType
{
    float4 svpos : SV_POSITION;
};

cbuffer cbuff0 : register(b0)
{
    matrix mat; // �ϊ��s��
    float4 baseColor; // ���ǉ��F�x�[�X�J���[�i�l�p�p�E�ۗp�ŕς���j
};
