#include <Windows.h>
#include <string>
#include <list>
#include <algorithm>

#include "Snap.h"
#include "RectHelper.h"

#pragma data_seg(".shared")

// Hook handle, shared among all injected DLLs.
HHOOK hookHandle = nullptr;

#pragma data_seg()

WNDPROC oldSubclassProc;
// Is the hook enabled for this process?
bool enabled;
Snap snapper;

LRESULT CALLBACK TemporarySubclassProc(HWND window, int message, WPARAM wparam, LPARAM lparam)
{
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR) oldSubclassProc);

	if (snapper.WillHandleMessage(message))
	{
		if (message == WM_MOVING || message == WM_SIZING)
		{
			RECT& bounds = *reinterpret_cast<RECT*>(lparam);
			RECT oldBounds = bounds;

			// If the window already handles the message, back off.
			LRESULT result = CallWindowProc(oldSubclassProc, window, message, wparam, lparam);
			if (result != 0 || bounds != oldBounds)
				return result;

			return snapper.HandleMessage(window, message, wparam, lparam) ? 1 : 0;
		}
		else
			snapper.HandleMessage(window, message, wparam, lparam);
	}

	return CallWindowProc(oldSubclassProc, window, message, wparam, lparam);
}

extern "C"
{
	// The CallWndProc procedure passed into SetWindowsHookEx
	LRESULT CALLBACK HookProc(int code, WPARAM wparam, LPARAM lparam)
	{
		auto cwp = reinterpret_cast<CWPSTRUCT*>(lparam);

		if (enabled && code == HC_ACTION)
		{
			if (snapper.WillHandleMessage(cwp->message))
			{
				oldSubclassProc = (WNDPROC) SetWindowLongPtr(cwp->hwnd, GWLP_WNDPROC, (LONG_PTR) TemporarySubclassProc);
			}

			return 0;
		}
		else
		{
			// If hookHandle hasn't been set yet, oh well; there's nothing we can do about it.
			return CallNextHookEx(hookHandle, code, wparam, lparam);
		}
	}

	void SetHookHandle(HHOOK handle)
	{
		hookHandle = handle;
	}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
		wchar_t filename[MAX_PATH + 1];
		int result = GetModuleFileName(nullptr, filename, MAX_PATH);
		if (result > 0)
		{
			// Get the filename without path and normalize it to lowercase for comparison.
			filename[result] = '\0';
			wchar_t baseFilename[MAX_PATH + 1];
			wchar_t* lastSlash = wcsrchr(filename, '\\');
			wcscpy_s(baseFilename, lastSlash + 1);
			_wcslwr_s(baseFilename);

			// No special cases at present.
		}

		// Enable for all processes
		enabled = true;
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		break;
	}
	}
	return TRUE;
}