// =========================================================
// gamebg.h �Q�[���V�[������
// 
// �����:		���t�F
// =========================================================
#ifndef _GAMEBG_H_
#define _GAMEBG_H_
void InitializeGamebg(void);
void UpdateGamebg(void);
void DrawGamebg(void);
void FinalizeGamebg(void);

// =========================================================
// �\���̐錾
// =========================================================
struct GAMEBG {
	Float2 pos;		// ���W
	Float2 size;	// �T�C�Y
	bool use;		// �g�p�t���O
};

#endif