
#pragma once
// main.h
#ifndef MAIN_H
#define MAIN_H

#include "system.h"


#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

//#define		SCREEN_WIDTH	(1280)
//#define		SCREEN_HEIGHT	(720)

#define NUM_VERTEX_QUADS		(4)	// 頂点数(四角形)

struct Float2
{
	float x, y, w;
};

struct Float3
{
	float x, y, z, w;
};

struct Float4
{
	float x, y, z, w;
};

struct Vec2 {
	float x, y;
};

struct Polygon_ {
	std::vector<Vec2> vertices;
	std::vector<int> faces;
};

// 頂点情報
struct VERTEX_3D
{
	Float3 Position;	// 座標
	Float4 Color;		// 色
	Float2 TexCoord;	// テクスチャ座標
};

Float2 MakeFloat2(float x, float y);

Float3 MakeFloat3(float x, float y, float z);

Float4 MakeFloat4(float x, float y, float z, float w);


// シーン遷移管理用
enum SCENE {
	SCENE_TITLE = 0,
	SCENE_GAME,
	SCENE_RESULT = 5,
	SCENE_MAX
};


// =========================================================
// シーン切り替え
// =========================================================
void SetScene(SCENE set);

SCENE CheckScene(void);

#endif // !MAIN_H