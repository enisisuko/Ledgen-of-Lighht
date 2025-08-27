// ===================================================
// collision.h 当たり判定
// 
// 制作者：				日付：2024
// ===================================================
#ifndef _COLLISION_H_
#define _COLLISION_H_

// ===================================================
// プロトタイプ宣言
// ===================================================
bool CheckBoxCollider(Float2 PosA, Float2 PosB, Float2 SizeA, Float2 SizeB);	// バウンディングボックスの当たり判定
bool CheckCircleCollider(Float2 PosA, Float2 PosB, float rA, float rB);		// バウンディングサークルの当たり判定

bool PointInTriangle(const Float2& p, const Float2& a, const Float2& b, const Float2& c);

bool PointInQuad(const Float2& p, const Float2 quad[4]);


#endif