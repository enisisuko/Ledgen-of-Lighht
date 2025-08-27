// ===================================================
// collision.cpp 当たり判定
// 
// 制作者：				日付：2024
// ===================================================
#include "main.h"
#include "collision.h"

// ===================================================
// バウンディングボックスの当たり判定
// 
// 引数:
// 	矩形Ａの中心座標
// 	矩形Ｂの中心座標
// 	矩形Ａのサイズ
// 	矩形Ｂのサイズ
//
// 戻り値
//  true：当たっている
// 	false：当たっていない
// ===================================================
bool CheckBoxCollider(Float2 PosA, Float2 PosB, Float2 SizeA, Float2 SizeB)
{
	float ATop =	PosA.y - SizeA.y / 2;	// Aの上端
	float ABottom =	PosA.y + SizeA.y / 2;	// Aの下端
	float ARight =	PosA.x + SizeA.x / 2;	// Aの右端
	float ALeft =	PosA.x - SizeA.x / 2;	// Aの左端

	float BTop =	PosB.y - SizeB.y / 2;	// Bの上端
	float BBottom =	PosB.y + SizeB.y / 2;	// Bの下端
	float BRight =	PosB.x + SizeB.x / 2;	// Bの右端
	float BLeft =	PosB.x - SizeB.x / 2;	// Bの左端

	if ((ARight >= BLeft) &&		// Aの右端 > Bの左端
		(ALeft <= BRight) &&		// Aの左端 < Bの右端
		(ABottom >= BTop) &&		// Aの下端 > Bの上端
		(ATop <= BBottom))			// Aの上端 < Bの下端
	{
		// 当たっている
		return true;
	}

	// 当たっていない
	return false;
}

// ===================================================
// バウンディングサークルの当たり判定
// 
// 引数:
// 	円１の中心座標
// 	円２の中心座標
// 	円１の半径
// 	円２の半径
//
// 戻り値
//  true：当たっている
// 	false：当たっていない
// ===================================================
bool CheckCircleCollider(Float2 PosA, Float2 PosB, float rA, float rB)
{
	// (円Aの中心座標X - 円Bの中心座標X)の2乗 + (円Aの中心座標Y - 円Bの中心座標Y)の2乗
	float distance = (PosA.x - PosB.x) * (PosA.x - PosB.x) + (PosA.y - PosB.y) * (PosA.y - PosB.y);
	
	// (円1の半径+円2の半径)の2乗
	float rSum = (rA + rB) * (rA + rB);

	// 2つの円の距離が半径の合計を下回った時
	if (distance <= rSum)
	{
		// 当たっている
		return true;
	}

	// 当たっていない
	return false;
}

bool PointInTriangle(const Float2& p, const Float2& a, const Float2& b, const Float2& c)
{
    float area = abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y));
    float area1 = abs((a.x - p.x) * (b.y - p.y) - (b.x - p.x) * (a.y - p.y));
    float area2 = abs((b.x - p.x) * (c.y - p.y) - (c.x - p.x) * (b.y - p.y));
    float area3 = abs((c.x - p.x) * (a.y - p.y) - (a.x - p.x) * (c.y - p.y));

    return abs(area - (area1 + area2 + area3)) < 0.01f;
}

bool PointInQuad(const Float2& p, const Float2 quad[4])
{
    return PointInTriangle(p, quad[0], quad[1], quad[2]) ||
        PointInTriangle(p, quad[0], quad[2], quad[3]);
}


Float2 GetNormal(Float2 a, Float2 b) {
    Float2 edge = { b.x - a.x, b.y - a.y };
    return { -edge.y, edge.x };
}

void Project(const Float2* points, int count, Float2 axis, float& min, float& max) {
    min = max = points[0].x * axis.x + points[0].y * axis.y;
    for (int i = 1; i < count; ++i) {
        float p = points[i].x * axis.x + points[i].y * axis.y;
        if (p < min) min = p;
        if (p > max) max = p;
    }
}

bool CheckCollision_Rect_Triangle(Float2 rectPos, Float2 rectSize, Float2 tri[3]) {
    Float2 half = { rectSize.x / 2.0f, rectSize.y / 2.0f };

    Float2 rect[4] = {
        { rectPos.x - half.x, rectPos.y - half.y },
        { rectPos.x + half.x, rectPos.y - half.y },
        { rectPos.x + half.x, rectPos.y + half.y },
        { rectPos.x - half.x, rectPos.y + half.y }
    };

    Float2 axes[5] = {
        GetNormal(rect[0], rect[1]),
        GetNormal(rect[1], rect[2]),
        GetNormal(tri[0], tri[1]),
        GetNormal(tri[1], tri[2]),
        GetNormal(tri[2], tri[0]),
    };

    for (int i = 0; i < 5; ++i) {
        float len = std::sqrt(axes[i].x * axes[i].x + axes[i].y * axes[i].y);
        if (len != 0.0f) {
            axes[i].x /= len;
            axes[i].y /= len;
        }

        float minA, maxA, minB, maxB;
        Project(rect, 4, axes[i], minA, maxA);
        Project(tri, 3, axes[i], minB, maxB);
        if (maxA < minB || maxB < minA)
            return false;
    }

    return true;
}

bool CheckCollisionSAT(Float2 PosA, Float2 PosB, Float2 SizeA, Polygon_* SizeB)
{
    int num = 0;
    int num_max = SizeB->faces.size() / 3;

    for (int i = 0;i < num_max;i++)
    {
        Float2 tri[3];

        for (int l = 0; l < 3; l++)
        {
            tri[l].x = SizeB->vertices[SizeB->faces[i * 3 + l]].x + GetScreenOffsetX();
            tri[l].y = SizeB->vertices[SizeB->faces[i * 3 + l]].y + GetScreenOffsetY();
        }

        if(CheckCollision_Rect_Triangle(PosA, PosB, tri))
        {
            return true;
        }
    }

    return false;
}

