#pragma once
#include "BuildSystem.h"

// 这里只能是 extern 声明，不能定义
extern RTS9::GameState gBuild;

// 只在“第一次进入游戏”时初始化基建
bool EnsureBuildInitialized();
