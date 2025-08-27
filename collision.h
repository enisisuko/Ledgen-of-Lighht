// ===================================================
// collision.h �����蔻��
// 
// ����ҁF				���t�F2024
// ===================================================
#ifndef _COLLISION_H_
#define _COLLISION_H_

// ===================================================
// �v���g�^�C�v�錾
// ===================================================
bool CheckBoxCollider(Float2 PosA, Float2 PosB, Float2 SizeA, Float2 SizeB);	// �o�E���f�B���O�{�b�N�X�̓����蔻��
bool CheckCircleCollider(Float2 PosA, Float2 PosB, float rA, float rB);		// �o�E���f�B���O�T�[�N���̓����蔻��

bool PointInTriangle(const Float2& p, const Float2& a, const Float2& b, const Float2& c);

bool PointInQuad(const Float2& p, const Float2 quad[4]);


#endif