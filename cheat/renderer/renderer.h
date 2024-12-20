#pragma once

#include <d3d9.h>
#include <d3dx9.h>

#include "../math/vector.h"

#define WIDTH 2560
#define HEIGHT 1440	
#define WINDOW_W 2560
#define WINDOW_H 1440

namespace renderer
{
	inline IDirect3D9* pD3D = nullptr;
	inline IDirect3DDevice9* pDevice = nullptr;
	inline ID3DXLine* mLine = nullptr;
	inline ID3DXFont* pFont = nullptr;  

	inline MSG msg;
	inline bool running = true;
	extern bool wallhackEnabled;
	extern bool aimbotEnabled;

	bool init(HWND hwnd);
	void destroy();
	void frame();
	void handle_events();

	bool initFont(); 
	void releaseFont();  

	namespace draw
	{
		void line(D3DXVECTOR2 p1, D3DXVECTOR2 p2, D3DCOLOR color, float thickness = 1.0f);
		void box(D3DXVECTOR2 tl, D3DXVECTOR2 br, D3DCOLOR color, float thickness = 1.0f);
		void text(const wchar_t* message, int x, int y, D3DCOLOR color);  
	}
};

namespace window
{
	HWND InitWindow(HINSTANCE hInstance);
	inline LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT Result = 0;
		switch (message)
		{
		case WM_DESTROY:
		{
			renderer::running = false;
			break;
		}
		case WM_CLOSE:
		{
			renderer::running = false;
			break;
		}
		default:
		{
			Result = DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}

		}
		return Result;
	}


}


