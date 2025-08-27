#pragma once


#include	<sdkddkver.h>
#define		WIN32_LEAN_AND_MEAN

#include	<windows.h>
#include	"debug_ostream.h"

//’Ç‰Á
#include	<algorithm>
#include	"direct3d.h"
#include	"shader.h"
#include	"sprite.h"
//

#include	"GamepadManager _1.0/GamepadManager.h"
#include	"GamepadManager _1.0/GamepadManager_DEDUG.h"

#include	"Keyboard.h"
#include	"Mouse.h"

void InitSystem(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

void UninitSystem();

void SwapBuffers();

HWND* GethWnd(void);

extern MSG msg;


extern GamepadDetectorManager manager;

extern Keyboard kbd;
extern Mouse mouse;