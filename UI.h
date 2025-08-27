#pragma once

// UI.h
enum class UIState
{
    Collapsed,  // 折りたたみ（合并）
    Expanded    // 展開（展开）
};



void InitializeUi(void);
void UpdateUi(void);
void DrawUi(void);
void FinalizeUi(void);

Object_2D** GetUi(void);

UIState* GetUiState(void);