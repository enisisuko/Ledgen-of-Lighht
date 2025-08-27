// main.cpp

#include "main.h"
#include "texture.h"
#include "sprite.h"
#include "controller.h"
#include "game.h"
#include <iomanip>
#include "ChiliTimer/ChiliTimer.h"
#include "BuildGlobals.h"
#include "BuildSystem.h"


// =========================================================
// プロトタイプ宣言
// =========================================================
void Initialize(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void Update(void);
void Draw(void);
void Finalize(void);

ChiliTimer timer;

SCENE scene;

#ifdef _DEBUG
int g_CountFPS;
char g_DebugStr[2048];

#endif // _DEBUG

#pragma comment(lib,"winmm.lib")

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	// 初期化
	Initialize(hInstance, hPrevInstance, lpCmdLine, nCmdShow);


	float targetFps = 60.0f;
	float minFrameTime = (targetFps > 0.0f) ? 1.0f / targetFps : 0.0f;

	float accumulatedTime = 0.0f;

	/*while (GetMessage(&msg, NULL, 0, 0))*/
	do
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
		


			float dt = timer.Mark(); // 当前帧耗时（秒）
			accumulatedTime += dt;
			// ✅ 如果帧间隔足够，就执行一次绘制
			if (targetFps == 0.0f || accumulatedTime >= minFrameTime)
			{



				float fps = 1.0f / accumulatedTime;

				accumulatedTime = 0.0f;

				static float smoothedFps = 60.0f;
				smoothedFps = smoothedFps * 0.9f + fps * 0.1f; // 慢慢接近新值，避免跳动
				//smoothedFps = fps * 0.1f; // 慢慢接近新值，避免跳动
				std::wostringstream oss;
				oss << L"FPS: " << std::fixed << std::setprecision(1) << smoothedFps;

				std::wstring hud;
				{
					std::string s = RTS9::HUDText(gBuild);
					hud = std::wstring(s.begin(), s.end());
				}
				oss << L" | " << hud;
				// 在 main.cpp 的标题拼接处，加在 HUD 之后：
				oss << L" | MENU:" << (gBuild.menuOpen ? L"OPEN" : L"CLOSED");

				SetWindowTextW(*GethWnd(), oss.str().c_str());


				SetWindowTextW(*GethWnd(), oss.str().c_str());
				// ✔ 处理输入、更新逻辑、绘制
				// 描画
					// 更新
				Update();
				Draw();
			}
			

			
		}
	} while (msg.message != WM_QUIT);



	// 終了処理
	Finalize();


	return (int)msg.wParam;
}



// =========================================================
// 初期化
// =========================================================
void Initialize(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// システム系初期化
	InitSystem(hInstance, hPrevInstance, lpCmdLine, nCmdShow);



	/*constexpr int kClientW = 1600;
	constexpr int kClientH = 900;

	HWND hWnd = *GethWnd();
	RECT rc{ 0,0,kClientW,kClientH };
	DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
	DWORD ex = (DWORD)GetWindowLongPtr(hWnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&rc, style, FALSE, ex);
	SetWindowPos(hWnd, nullptr, 0, 0,
		rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOMOVE | SWP_NOZORDER);
*/





	InitSprite();
	InitController();

	// オブジェクト初期化
	// シーンの初期化
	scene = SCENE_GAME;

	// オブジェクト初期化	
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		InitializeGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}


}

// =========================================================
// 更新
// =========================================================
void Update(void)
{
	// システム系更新
	UpdateController();

	// オブジェクト更新
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		UpdateGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}


}

// =========================================================
// 描画
// =========================================================
void Draw(void)
{
	//// クリア色設定
	//glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

	//// 画面クリア
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//// オブジェクト描画
	

	//glFinish();
	//SwapBuffers();

	Direct3D_Clear();

	Polygon_Draw();
	// オブジェクト描画
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		DrawGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}


	Direct3D_Present();
}

// =========================================================
// 終了処理
// =========================================================
void Finalize(void)
{
	// オブジェクト終了処理
	// オブジェクト終了処理
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		FinalizeGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}



	// システム系終了処理
	UninitController();
	UninitSprite();
	UninitSystem();


}


// =========================================================
// シーン切り替え
// =========================================================
void SetScene(SCENE set)
{
	// オブジェクト終了処理
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		FinalizeGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}

	// 切り替え
	scene = set;

	// オブジェクト初期化	
	switch (scene)
	{
	case SCENE_TITLE:

		break;
	case SCENE_GAME:
		InitializeGame();
		break;
	case SCENE_RESULT:

		break;
	case SCENE_MAX:
		break;
	default:
		break;
	}
}

SCENE CheckScene(void)
{
	return scene;
}


#include "BuildGlobals.h"
#include "BuildSystem.h"

RTS9::GameState gBuild;
static bool gInited = false;

bool EnsureBuildInitialized()
{
	if (!gInited) {
		RTS9::InitGame(gBuild); // 只在第一次进入游戏时清空
		gInited = true;
		return true;            // 首次做了初始化
	}
	return false;               // 已经初始化过，不再清空
}
