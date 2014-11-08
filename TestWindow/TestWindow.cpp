#include <Windows.h>
#include <cmath>
#include <string>
#include <list>
#include <algorithm>
#include <array>

#include "../Hook/Snap.h"

bool InitInstance(HINSTANCE instance);
LRESULT CALLBACK WinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
void CreateGameWindow(int width, int height, bool fullScreen);

int WINAPI wWinMain(HINSTANCE, HINSTANCE instance, LPWSTR, int)
{
	InitInstance(instance);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

bool InitInstance(HINSTANCE instance)
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpszClassName = L"TestWindowClass";
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WinProc;
	wc.hbrBackground = (HBRUSH) ((int) GetStockObject(WHITE_BRUSH) + 1);

	bool success = (RegisterClassEx(&wc) != 0);
	if (!success)
		return false;

	DWORD exStyles = WS_EX_OVERLAPPEDWINDOW;
	DWORD styles = WS_VISIBLE | WS_OVERLAPPEDWINDOW;

	RECT rc;
	rc.left = 0; rc.top = 0; rc.right = 800; rc.bottom = 600;	// fix

	AdjustWindowRectEx(&rc, styles, false, exStyles);	// check

	HWND window = CreateWindowEx(exStyles, wc.lpszClassName, L"Test", styles, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, HWND_DESKTOP, nullptr, instance, nullptr);

	return true;
}

bool enabled;
RECT _bounds;
POINT originalCursorOffset = { 0 };
POINT cursorOffset;
RECT realBounds = { 0 };
bool inProgress = false;
int resetCount = 0;
WPARAM lastwParam = 0;
bool snapLeft;
bool snapTop;
bool snapRight;
bool snapBottom;

Snap snapper;

LRESULT CALLBACK WinProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC context = BeginPaint(window, &ps);

		wchar_t buffer[256];
		swprintf_s(buffer, L"cursorOffset: %d, %d\noriginalCursorOffset: %d, %d\nbounds: %d, %d, %d, %d\nrealBounds: %d, %d, %d, %d\ninProgress: %s\nsnaps: %d %d %d %d", cursorOffset.x, cursorOffset.y, originalCursorOffset.x, originalCursorOffset.y, _bounds.left, _bounds.top, _bounds.right, _bounds.bottom, realBounds.left, realBounds.top, realBounds.right, realBounds.bottom, inProgress ? L"yes" : L"no", snapLeft, snapTop, snapRight, snapBottom);

		RECT rc = { 0, 0, 800, 600 };
		DrawText(context, buffer, -1, &rc, DT_LEFT);

		EndPaint(window, &ps);
		break;
	}

	case WM_MOVING:
	{
		lastwParam = wParam;
		InvalidateRect(window, nullptr, TRUE);

		snapper.HandleMessage(window, message, wParam, lParam);

		return DefWindowProc(window, message, wParam, lParam);
	}

	case WM_SIZING:
	{
		lastwParam = wParam;
		InvalidateRect(window, nullptr, TRUE);
		
		snapper.HandleMessage(window, message, wParam, lParam);

		return DefWindowProc(window, message, wParam, lParam);
	}

	case WM_ENTERSIZEMOVE:
		snapper.HandleMessage(window, message, wParam, lParam);
		break;

	case WM_EXITSIZEMOVE:
		snapper.HandleMessage(window, message, wParam, lParam);
		break;

	case WM_CLOSE:
		DestroyWindow(window);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(window, message, wParam, lParam);
	}

	return 0;
}