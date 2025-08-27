// =========================================================
// game.h ゲームシーン制御
// 
// 制作者:		日付：
// =========================================================
#ifndef _GAME_H_
#define _GAME_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif


// =========================================================
// マクロ宣言
// =========================================================
enum class GameScene
{
	Ready,     // 准备阶段
	Battle,    // 战斗阶段
	Result     // 结算阶段
};

void InitializeGame(void);
void UpdateGame(void);
void DrawGame(void);
void FinalizeGame(void);

GameScene GetcurrentScene(void);
void SetcurrentScene(GameScene type);

int GetMoney(void);
int GetPlayers(void);

void ADDMoney(int i);
void ADDPlayers(int i);

void SUBMoney(int i);
void SUBPlayers(int i);

#endif