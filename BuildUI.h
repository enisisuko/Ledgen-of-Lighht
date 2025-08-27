#pragma once
// BuildUI.h
#pragma once
#include "BuildSystem.h"
void InitBuildUI();
void UninitBuildUI();

// 在地图上把建筑图标画出来
void DrawBuildWorld(const RTS9::GameState& gs);

// 画扇形菜单（围绕玩家位置）
void DrawBuildMenu(const RTS9::GameState& gs);

void BuildUI_ResetMenuState();
void BuildUI_Tick();                               // 只更新，不绘制
void BuildUI_DrawTopMost(const RTS9::GameState&);