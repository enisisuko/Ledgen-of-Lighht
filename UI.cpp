// UI.cpp
#include	"main.h"
#include	"UI.h"
#include	"texture.h"
#include "Map.h"


unsigned int UiId[10];//テクスチャID

Object_2D* Uis[12];

Object_2D::Vec2 pos;


UIState state = UIState::Collapsed;  // 初期状態

void InitializeUi(void)
{
	UiId[0] = LoadTexture("rom:/UI/iconmonstr-caret-up-lined-240.tga");
	UiId[1] = LoadTexture("rom:/UI/iconmonstr-caret-up-filled-240.tga");
	UiId[2] = LoadTexture("rom:/bg_01.jpg");
	
	Uis[0] = new Object_2D(0.0f, 500.0f, 100.0f, 100.0f);
	Uis[1] = new Object_2D(0.0f, 650.0f, 1920.0f, 200.0f);

	Uis[0]->texNo = UiId[0];

	Uis[1]->texNo = UiId[2];

	for (int i = 0;i < 10;i++)
	{
		Uis[2 + i] = new Object_2D(-900.0f+i*200.0f, 650.0f, 100.0f, 100.0f);

	}
}

void UpdateUi(void)
{
	if (state == UIState::Expanded)
	{
		if (pos.y < 200)
		{
			pos.y += 20;
		}
		else
		{
			pos.y = 200;
		}
		for (int i = 0;i < 10;i++)
		{
			Uis[2 + i]->texNo = GetMapTexturesID()[i];
			
		}
		
		
	}
	else if (state == UIState::Collapsed)
	{
		if (pos.y > 0)
		{
			pos.y -= 20;
		}
	}
}

void DrawUi(void)
{
	Uis[0]->Transform.Position.y = 500.0f - pos.y;
	Uis[1]->Transform.Position.y = 650.0f - pos.y;

	BoxVertexsMake(Uis[0]);
	DrawObject_2D(Uis[0]);

	BoxVertexsMake(Uis[1]);
	DrawObject_2D(Uis[1]);

	for (int i = 0;i < 10;i++)
	{
		Uis[2 + i]->Transform.Position.y = 650.0f - pos.y;
		Uis[2 + i]->texNo = GetMapTexturesID()[i];

		BoxVertexsMake(Uis[2 + i]);
		DrawObject_2D(Uis[2 + i]);
	}


}

void FinalizeUi(void)
{
	for (Object_2D* obj : Uis)
	{
		delete obj;
	}
}

Object_2D** GetUi(void)
{
	return Uis;
}

Object_2D::Vec2 GetUiPos(void)
{
	return pos;
}

void SetUiPos(int x,int y)
{
	pos.x = x;
	pos.y = y;
}

UIState* GetUiState(void)
{
	return &state;
}