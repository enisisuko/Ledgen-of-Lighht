#include "main.h"

#define		CLASS_NAME "DX21 Window"
#define		WINDOW_CAPTION "DX21ウィンド表示"

MSG msg;
HWND hWnd;

GamepadDetectorManager manager;

Keyboard kbd;
Mouse mouse; 

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void InitSystem(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);


	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = CLASS_NAME;
	wc.hIconSm = nullptr;

	RegisterClassEx(&wc);


	RECT	window_rect = { 0,0, SCREEN_WIDTH,SCREEN_HEIGHT };

	DWORD window_style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);

	AdjustWindowRect(&window_rect, window_style, FALSE);

	int window_width = window_rect.right - window_rect.left;
	int window_height = window_rect.bottom - window_rect.top;

	hWnd = CreateWindow(
		CLASS_NAME,
		WINDOW_CAPTION,
		window_style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		window_width,
		window_height,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);
	Direct3D_Initialize(hWnd);

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

	

	hal::dout << "スタート\n" << std::endl;

	manager.run();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HGDIOBJ hbrWhite, hbrGray;
	HDC		hdc;
	PAINTSTRUCT	ps;


	switch (uMsg)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);



		EndPaint(hWnd, &ps);
		return 0;
	//case WM_KEYDOWN:
	//	if (wParam == VK_ESCAPE)
	//	{
	//		SendMessage(hWnd, WM_CLOSE, 0, 0);
	//	}
	//	break;
	case WM_CLOSE:
		if (MessageBoxA(hWnd, "本当に終了してよろしいでしょうか？", "確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK)
		{
			DestroyWindow(hWnd);
		}
		else
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;


	case WM_KEYDOWN:
		if (!(lParam & 0x40000000) || kbd.AutorepeatIsEnabled())
		{
			kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		return 0;

	case WM_KEYUP:
		kbd.OnKeyReleased(static_cast<unsigned char>(wParam));
		return 0;

	case WM_CHAR:
		kbd.OnChar(static_cast<unsigned char>(wParam));
		return 0;

	case WM_KILLFOCUS:
		kbd.ClearState();
		break;

	case WM_MOUSEMOVE:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		mouse.OnMouseMove(pt.x, pt.y);
		if (pt.x >= 0 && pt.x < SCREEN_WIDTH && pt.y >= 0 && pt.y < SCREEN_HEIGHT)
		{
			if (!mouse.IsInWindow())
			{
				SetCapture(hWnd);
				mouse.OnMouseEnter();
			}
		}
		else
		{
			if (wParam & (MK_LBUTTON | MK_RBUTTON))
			{
				mouse.OnMouseMove(pt.x, pt.y);
			}
			else
			{
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
		}
		return 0;
	}
	case WM_LBUTTONDOWN:
		mouse.OnLeftPressed(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_LBUTTONUP:
		mouse.OnLeftReleased(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		mouse.OnRightPressed(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_RBUTTONUP:
		mouse.OnRightReleased(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_MBUTTONDOWN:
		mouse.OnMiddlePressed(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_MBUTTONUP:
		mouse.OnMiddleReleased(LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_MOUSEWHEEL:
	{
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		mouse.OnWheelDelta(pt.x, pt.y, delta);
		return 0;
	}

	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void UninitSystem()
{
	Shader_Finalize();

	Direct3D_Finalize();
}

HWND* GethWnd(void)
{
	return &hWnd;
}



