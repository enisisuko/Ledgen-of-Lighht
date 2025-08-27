// =========================================================
// gamebg.h ゲームシーン制御
// 
// 制作者:		日付：
// =========================================================
#ifndef _GAMEBG_H_
#define _GAMEBG_H_
void InitializeGamebg(void);
void UpdateGamebg(void);
void DrawGamebg(void);
void FinalizeGamebg(void);

// =========================================================
// 構造体宣言
// =========================================================
struct GAMEBG {
	Float2 pos;		// 座標
	Float2 size;	// サイズ
	bool use;		// 使用フラグ
};

#endif