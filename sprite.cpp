// sprite.cpp

#include "sprite.h"


#include "main.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include "texture.h"


#include "direct3d.h"
#include "shader.h"
#include "debug_ostream.h"

unsigned int	g_SpriteVertexArrayObject;
unsigned int	g_SpriteVertexBuffer;

Float2 ScreenOffset;





static constexpr int NUM_VERTEX = 6; // 使用できる最大頂点数


static ID3D11Buffer* g_pVertexBuffer = nullptr; // 頂点バッファ


// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;

ID3D11Buffer* g_pConstantBuffer = nullptr;


void Polygon_Draw(void)
{
	// シェーダーを描画パイプラインに設定
	Shader_Begin();

	// 頂点情報を書き込み
	const float SCREEN_WIDTH_ = (float)Direct3D_GetBackBufferWidth();
	const float SCREEN_HEIGHT_ = (float)Direct3D_GetBackBufferHeight();

	Shader_SetMatrix(DirectX::XMMatrixOrthographicOffCenterLH(
		0.0f,
		SCREEN_WIDTH_,
		SCREEN_HEIGHT_,
		0.0f,
		0.0f,
		1.0f));
}

// 頂点構造体
struct Vertex
{
	DirectX::XMFLOAT3 Position; // 頂点座標
	DirectX::XMFLOAT4 Color = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT2 TexCoord;
};

void DrawObject_2D(Object_2D* obj_2D)
{
	ID3D11ShaderResourceView* g_Texture = SetTexture(obj_2D->texNo);
	if (g_Texture != NULL)
	{
		g_pContext->PSSetShaderResources(0, 1, &g_Texture);
	}
	else
	{
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		g_pContext->PSSetShaderResources(0, 1, nullSRV);
	}

	SetBlendState(BLENDSTATE_ALFA);

	// 頂点バッファをロックする
	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

	// 頂点バッファへの仮想ポインタを取得
	Vertex* vertexQuad = (Vertex*)msr.pData;

	//// 画面の左上から右下に向かう線分を描画する
	vertexQuad[0].Position = { obj_2D->Vertexs[0].x, obj_2D->Vertexs[0].y, 0.0f };			// 一番右の0.0を削除
	vertexQuad[1].Position = { obj_2D->Vertexs[1].x, obj_2D->Vertexs[1].y, 0.0f };	//　一番右の0.0を削除
	vertexQuad[2].Position = { obj_2D->Vertexs[2].x, obj_2D->Vertexs[2].y, 0.0f };	//　一番右の0.0を削除
	vertexQuad[3].Position = { obj_2D->Vertexs[3].x, obj_2D->Vertexs[3].y, 0.0f };	//　一番右の0.0を削除

	vertexQuad[0].Color = { obj_2D->Color.R, obj_2D->Color.G, obj_2D->Color.B, obj_2D->Color.A };
	vertexQuad[1].Color = { obj_2D->Color.R, obj_2D->Color.G, obj_2D->Color.B, obj_2D->Color.A };
	vertexQuad[2].Color = { obj_2D->Color.R, obj_2D->Color.G, obj_2D->Color.B, obj_2D->Color.A };
	vertexQuad[3].Color = { obj_2D->Color.R, obj_2D->Color.G, obj_2D->Color.B, obj_2D->Color.A };

	float u0 = obj_2D->flipX ? obj_2D->UV[1].x : obj_2D->UV[0].x ;
	float u1 = obj_2D->flipX ? obj_2D->UV[0].x : obj_2D->UV[1].x ;
	float v0 = obj_2D->flipY ? obj_2D->UV[2].y : obj_2D->UV[0].y ;
	float v1 = obj_2D->flipY ? obj_2D->UV[0].y : obj_2D->UV[2].y ;

	vertexQuad[0].TexCoord = { u0, v0 };
	vertexQuad[1].TexCoord = { u1, v0 };
	vertexQuad[2].TexCoord = { u0, v1 };
	vertexQuad[3].TexCoord = { u1, v1 };
	

	//	gamebg.size = { SCREEN_WIDTH, SCREEN_HEIGHT };		// サイズ
	for (int i = 0;i < 4;i++)
	{
		vertexQuad[i].Position.x += SCREEN_WIDTH / 2;
		vertexQuad[i].Position.y += SCREEN_HEIGHT / 2;
	}
	// 頂点バッファのロックを解除
	g_pContext->Unmap(g_pVertexBuffer, 0);

	// 頂点バッファを描画パイプラインに設定
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	// プリミティブトポロジ設定
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	// ポリゴン描画命令発行
	g_pContext->Draw(4, 0);

}


void InitSprite()
{
	

	// デバイスとデバイスコンテキストのチェック
	if (!Direct3D_GetDevice() || !Direct3D_GetDeviceContext()) {
		hal::dout << "Polygon_Initialize() : 与えられたデバイスかコンテキストが不正です" << std::endl;
		return;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = Direct3D_GetDevice();
	g_pContext = Direct3D_GetDeviceContext();

	// 頂点バッファ生成
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex) * NUM_VERTEX;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	g_pDevice->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	// ==== 追加：常量バッファ生成 ====
	struct ConstantBuffer
	{
		DirectX::XMMATRIX mtx;
		DirectX::XMFLOAT2 screenSize;
		DirectX::XMFLOAT2 padding;
	};

	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DEFAULT;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA cbInitData = {};
	ConstantBuffer cb = {};
	cb.mtx = DirectX::XMMatrixIdentity();
	cb.screenSize = { 1280.0f, 720.0f };
	cbInitData.pSysMem = &cb;

	HRESULT hr = g_pDevice->CreateBuffer(&cbd, &cbInitData, &g_pConstantBuffer);
	if (FAILED(hr)) {
		OutputDebugStringA("常量バッファ作成失敗\n");
	}

	// 頂点着色器にセット（slot 0）
	g_pContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);
}


void UninitSprite()
{
	SAFE_RELEASE(g_pVertexBuffer);
	std::vector<ID3D11ShaderResourceView*>* g_Textures = GetTexture();
	if (g_Textures->size() != 0)
	{
		for (int i = 0;i < g_Textures->size();i++)
		{
			(*g_Textures)[i]->Release();
		}
	}
	g_Textures->clear();
	if (g_pConstantBuffer)
	{
		g_pConstantBuffer->Release();
	}

}

//////////////////////////////

void InitializeScreenOffset(void)
{
	ScreenOffset = MakeFloat2(0.0f, 0.0f);
}


void SetScreenOffset(float x, float y)
{

	ScreenOffset.x += x;
	ScreenOffset.y -= y;
	/*if (ScreenOffset.x > 0)
	{
		ScreenOffset.x = 0.0f;
	}*/
}

float GetScreenOffsetX(void)
{
	return ScreenOffset.x;
}

float GetScreenOffsetY(void)
{
	return ScreenOffset.y;
}

Object_2D::Object_2D(float x, float y, float w, float h)
{
	Transform.Position.x = x;
	Transform.Position.y = y;

	Size.x = w;
	Size.y = h;

	Vertexs[0] = { Transform.Position.x - Size.x / 2, Transform.Position.y - Size.y / 2};			// 一番右の0.0を削除
	Vertexs[1] = { Transform.Position.x + Size.x / 2, Transform.Position.y - Size.y / 2};	//　一番右の0.0を削除
	Vertexs[2] = { Transform.Position.x - Size.x / 2, Transform.Position.y + Size.y / 2};	//　一番右の0.0を削除
	Vertexs[3] = { Transform.Position.x + Size.x / 2, Transform.Position.y + Size.y / 2};	//　一番右の0.0を削除

	UV[0] = { 0.0f,0.0f };
	UV[1] = { 1.0f,0.0f };
	UV[2] = { 0.0f,1.0f };
	UV[3] = { 1.0f,1.0f };
}

void Object_2D_UV(Object_2D* obj_2D,int bno, int wc, int hc)
{
#define	YOKO (10)
#define TEX_w (1.0f / YOKO)

	float w = 1.0f / wc;
	float h = 1.0f / hc;

	float x = (bno % wc) * w;
	float y = (bno / wc) * h;

	obj_2D->UV[0]= { x,y };
	obj_2D->UV[1]= { x + w,y };
	obj_2D->UV[2]= { x,y + h };
	obj_2D->UV[3]= { x + w,y + h };

}


void BoxVertexsMake(Object_2D* obj_2D)
{
	obj_2D->Vertexs[0] = { obj_2D->Transform.Position.x - obj_2D->Size.x / 2, obj_2D->Transform.Position.y - obj_2D->Size.y / 2 };			// 一番右の0.0を削除
	obj_2D->Vertexs[1] = { obj_2D->Transform.Position.x + obj_2D->Size.x / 2, obj_2D->Transform.Position.y - obj_2D->Size.y / 2 };	//　一番右の0.0を削除
	obj_2D->Vertexs[2] = { obj_2D->Transform.Position.x - obj_2D->Size.x / 2, obj_2D->Transform.Position.y + obj_2D->Size.y / 2 };	//　一番右の0.0を削除
	obj_2D->Vertexs[3] = { obj_2D->Transform.Position.x + obj_2D->Size.x / 2, obj_2D->Transform.Position.y + obj_2D->Size.y / 2};	//　一番右の0.0を削除
}