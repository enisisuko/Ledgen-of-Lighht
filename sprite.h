#pragma once

// sprite.h

class Object_2D
{
public:
	Object_2D(float x,float y,float w,float h);
	class Vec2
	{
	public:
		float x, y;
	};
	class Transform_2D
	{
	public:
		Vec2 Position;
		float Rotation = 1.0f;
		Vec2 Scale = { 1.0f,1.0f };

	};
	class ColorRGBA
	{
	public:
		float R = 1.0f;
		float G = 1.0f;
		float B = 1.0f;
		float A = 1.0f;
	};

	Vec2 Size;
	Transform_2D Transform;
	Vec2 UV[4];
	Vec2 Vertexs[4];
	ColorRGBA Color;
	unsigned int texNo = -1;

	bool flipX = false;
	bool flipY = false;
};
void Object_2D_UV(Object_2D* obj_2D, int bno, int wc, int hc);
void DrawObject_2D(Object_2D* obj_2D);
void BoxVertexsMake(Object_2D* obj_2D);

// プロトタイプ宣言
void InitSprite();
void UninitSprite();

void Polygon_Draw(void);

void InitializeScreenOffset(void);


void SetScreenOffset(float x, float y);

float GetScreenOffsetX(void);

float GetScreenOffsetY(void);