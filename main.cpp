#include <Windows.h>
#include "wc.h"

LRESULT CALLBACK WinProc(HWND windowHandler, UINT umessage, WPARAM wparam, LPARAM lparam) {
	if (umessage == WM_CREATE)
	{
		const auto* const createStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
		::SetWindowLongPtr(windowHandler, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
	}
	switch (umessage)
	{
		case WM_DESTROY:
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return DefWindowProc(windowHandler, umessage, wparam, lparam);
		}
	}
}

int Run()
{
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, PSTR pCmdLine, int nShowCmd)
{
	WindowClass win(hInstance, WinProc);
	win.CreateWinEx();
	win.ShowWin();

	return Run();
}