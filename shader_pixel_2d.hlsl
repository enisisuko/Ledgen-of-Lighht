/*==============================================================================

   2D描画用ピクセルシェーダー [shader_pixel_2d.hlsl]
--------------------------------------------------------------------------------
==============================================================================*/

//struct PS_INPUT
//{
//    float4 posH : SV_POSITION; //ピクセルの座標
//    float4 color : COLOR0; //ピクセルの色
//};

//float4 main(PS_INPUT ps_in) : SV_TARGET
//{
//    return ps_in.color; //色をそのまま出力
//}



//float4 main() : SV_TARGET
//{
//    return float4(1.0f, 1.0f, 1.0f, 1.0f);
//}

float4 parameter;

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

struct PS_INPUT
{
    float4 posH : SV_Position;
    float4 color : COLOR0;
    float2 texcorrd : TEXCOORD0;
};

float4 main(PS_INPUT ps_in):SV_Target
{
    float4 col;
    col = g_Texture.Sample(g_SamplerState, ps_in.texcorrd);
    col *= ps_in.color;
    
    return col;
}