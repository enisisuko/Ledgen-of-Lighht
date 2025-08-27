
#ifndef DIRECT3D_H
#define DIRECT3D_H



#pragma once


#include <Windows.h>
#include <d3d11.h>

#include "DirectXTex.h"

#if _DEBUG
#pragma comment(lib,"DirectXTex_Debug.lib")
#else
#pragma comment(lib,"DirectXTex_Release.lib")
#endif // _DEBUG



#define SAFE_RELEASE(o) if (o) { (o)->Release(); o=NULL; }

bool Direct3D_Initialize(HWND hWnd);

void Direct3D_Finalize();

void Direct3D_Clear();

void Direct3D_Present();


ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetDeviceContext();


unsigned int Direct3D_GetBackBufferWidth();
unsigned int Direct3D_GetBackBufferHeight();


enum BLENDSTATE
{
	BLENDSTATE_NONE = 0,
	BLENDSTATE_ALFA,
	BLENDSTATE_ADD,
	BLENDSTATE_SUB,

	BLENDSTATE_MAX,
};

void SetBlendState(BLENDSTATE blend);


#endif // !DIRECT3D_H