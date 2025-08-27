// ===================================================
// collision.cpp �����蔻��
// 
// ����ҁF				���t�F2024
// ===================================================
#include "main.h"
#include "collision.h"

// ===================================================
// �o�E���f�B���O�{�b�N�X�̓����蔻��
// 
// ����:
// 	��`�`�̒��S���W
// 	��`�a�̒��S���W
// 	��`�`�̃T�C�Y
// 	��`�a�̃T�C�Y
//
// �߂�l
//  true�F�������Ă���
// 	false�F�������Ă��Ȃ�
// ===================================================
bool CheckBoxCollider(Float2 PosA, Float2 PosB, Float2 SizeA, Float2 SizeB)
{
	float ATop =	PosA.y - SizeA.y / 2;	// A�̏�[
	float ABottom =	PosA.y + SizeA.y / 2;	// A�̉��[
	float ARight =	PosA.x + SizeA.x / 2;	// A�̉E�[
	float ALeft =	PosA.x - SizeA.x / 2;	// A�̍��[

	float BTop =	PosB.y - SizeB.y / 2;	// B�̏�[
	float BBottom =	PosB.y + SizeB.y / 2;	// B�̉��[
	float BRight =	PosB.x + SizeB.x / 2;	// B�̉E�[
	float BLeft =	PosB.x - SizeB.x / 2;	// B�̍��[

	if ((ARight >= BLeft) &&		// A�̉E�[ > B�̍��[
		(ALeft <= BRight) &&		// A�̍��[ < B�̉E�[
		(ABottom >= BTop) &&		// A�̉��[ > B�̏�[
		(ATop <= BBottom))			// A�̏�[ < B�̉��[
	{
		// �������Ă���
		return true;
	}

	// �������Ă��Ȃ�
	return false;
}

// ===================================================
// �o�E���f�B���O�T�[�N���̓����蔻��
// 
// ����:
// 	�~�P�̒��S���W
// 	�~�Q�̒��S���W
// 	�~�P�̔��a
// 	�~�Q�̔��a
//
// �߂�l
//  true�F�������Ă���
// 	false�F�������Ă��Ȃ�
// ===================================================
bool CheckCircleCollider(Float2 PosA, Float2 PosB, float rA, float rB)
{
	// (�~A�̒��S���WX - �~B�̒��S���WX)��2�� + (�~A�̒��S���WY - �~B�̒��S���WY)��2��
	float distance = (PosA.x - PosB.x) * (PosA.x - PosB.x) + (PosA.y - PosB.y) * (PosA.y - PosB.y);
	
	// (�~1�̔��a+�~2�̔��a)��2��
	float rSum = (rA + rB) * (rA + rB);

	// 2�̉~�̋��������a�̍��v�����������
	if (distance <= rSum)
	{
		// �������Ă���
		return true;
	}

	// �������Ă��Ȃ�
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

