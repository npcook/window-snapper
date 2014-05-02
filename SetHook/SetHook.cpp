#include <Windows.h>
#include <string>

HANDLE quitEvent;

DWORD CALLBACK ReadThread(void* param)
{
	HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
	char readBuffer[5];
	while (true)
	{
		DWORD bytesRead = 0;
		bool success = ReadFile(stdIn, readBuffer, 4, &bytesRead, nullptr);
		if (!success)
			break;
		readBuffer[bytesRead] = 0;

		if (bytesRead > 0)
		{
			if (strcmp(readBuffer, "QUIT") == 0)
				break;
		}
	}

	SetEvent(quitEvent);
	return 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	HMODULE hookModule;
#ifdef X64
	hookModule = LoadLibrary(L"Hook64.dll");
#else
	hookModule = LoadLibrary(L"Hook32.dll");
#endif
	HOOKPROC hookFunction = reinterpret_cast<HOOKPROC>(GetProcAddress(hookModule, "HookProc"));
	auto setHookHandle = reinterpret_cast<void(*)(HHOOK handle)>(GetProcAddress(hookModule, "SetHookHandle"));

	HHOOK hookHandle = SetWindowsHookEx(WH_CALLWNDPROC, hookFunction, hookModule, 0);
	setHookHandle(hookHandle);

	quitEvent = CreateEvent(nullptr, true, false, nullptr);
	CreateThread(nullptr, 0, ReadThread, nullptr, 0, nullptr);

	while (true)
	{
		if (WaitForSingleObject(quitEvent, 100) == WAIT_OBJECT_0)
			break;

		MSG message;
		while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	UnhookWindowsHookEx(hookHandle);

	DWORD_PTR result;
	SendMessageTimeout(HWND_BROADCAST, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000, &result);

	return 0;
}