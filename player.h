// =========================================================
// player.h プレイヤー制御
// 
// 制作者:		日付：
// =========================================================
#ifndef _PLAYER_H_
#define _PLAYER_H_
#pragma once
#include "sprite.h"

// =========================================================
// 構造体宣言
// =========================================================

void InitializePlayer(void);
void UpdatePlayer(void);
void DrawPlayer(void);
void FinalizePlayer(void);
Object_2D* GetPlayer(void);


bool Player_IsInitialized(void);
void Player_EnsureAllocated(void);

#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif