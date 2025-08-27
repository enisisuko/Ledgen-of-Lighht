/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
//float4x4 mtx; //C言語から渡されたデータが入っている


// 定数バッファ（C言語から渡される）
cbuffer CB0 : register(b0)
{
    float4x4 mtx; // 変換行列（使いたければ使える）
    float2 screenSize; // 画面サイズ（例: 1280, 720）
    float2 padding; // パディング（float4x4 に合わせるため）
}

//入力用頂点構造体
struct VS_INPUT
{ //              V コロン！
    float4 posL : POSITION0; //頂点座標 オーでなくゼロ！
    float4 color : COLOR0; //頂点カラー（R,G,B,A）
    float2 texcorrd : TEXCOORD0;
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; //変換済頂点座標
    float4 color : COLOR0; //頂点カラー
    float2 texcorrd : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    
    VS_OUTPUT vs_out; //出力用構造体変数

    // 頂点座標に行列を適用（必要なら）
    float4 transformed = mul(vs_in.posL, mtx);

    // ローカル→NDC変換（Switch 中央座標系 → DirectX 座標系）
    float2 ndc;
    ndc.x = transformed.x / (screenSize.x * 0.5f);
    ndc.y = -transformed.y / (screenSize.y * 0.5f); // Y軸反転
    
    //頂点を行列で変換
    vs_out.posH = mul(vs_in.posL, mtx);
    //vs_out.posH = float4(ndc, 0.0f, 1.0f);
    //頂点カラーはそのまま出力
    vs_out.color = vs_in.color;
    vs_out.texcorrd = vs_in.texcorrd;
    //結果を出力する
    return vs_out;
}



////=============================================================================
//// 頂点シェーダ
////=============================================================================
//float4 main(in float4 posL : POSITION0 ) : SV_POSITION
//{
//	return mul(posL, mtx);//頂点座標＊mtx（変換行列）
//}
