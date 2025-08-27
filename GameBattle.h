// =========================================================
// gamebattle.h プレイヤー制御
// 
// 制作者:		日付：
// =========================================================
#ifndef _GAMEBATTLE_H_
#define _GAMEBATTLE_H_

// =========================================================
// 構造体宣言
// =========================================================

void InitializeGameBattle(void);
void UpdateGameBattle(void);
void DrawGameBattle(void);
void FinalizeGameBattle(void);
// GameBattle.h （新增）
#pragma once
int Battle_GetWaveForHUD_Ready();
int Battle_GetWaveForHUD_Battle();

float Battle_GetFailFadeAlpha();   // 查询当前需要的黑幕透明度（0..1）

// 发射一个“我方火球”（非锁定直线飞行，带轻微散布）
// GameBattle.h
void Battle_SpawnAllyFireball(float sx, float sy, float dirx, float diry, float dmg);
void Battle_SpawnAllyFireballGiant(float sx, float sy, float dirx, float diry);



#endif