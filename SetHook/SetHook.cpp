#include <Windows.h>
#include <string>

DWORD CALLBACK ReadThread(void*)
{
	HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
	if (stdIn == INVALID_HANDLE_VALUE)
		return 1;
	char readBuffer[5];
	DWORD bytesRead = 0;
	bool done = false;
	// Read from standard input until it closes or we receive a QUIT message.
	while (!done)
	{
		bool success = ReadFile(stdIn, readBuffer, 4, &bytesRead, nullptr) != 0;
		if (!success)
			done = true;
		else if (bytesRead > 0)
		{
			readBuffer[bytesRead] = 0;
			if (strcmp(readBuffer, "QUIT") == 0)
				done = true;
		}
	}

	return 0;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
	HMODULE hookModule;
#ifdef _M_X64
	hookModule = LoadLibrary(L"Hook64.dll");
#else
	hookModule = LoadLibrary(L"Hook32.dll");
#endif
	HOOKPROC hookFunction = reinterpret_cast<HOOKPROC>(GetProcAddress(hookModule, "HookProc"));
	auto setHookHandle = reinterpret_cast<void(*)(HHOOK handle)>(GetProcAddress(hookModule, "SetHookHandle"));

	HHOOK hookHandle = SetWindowsHookEx(WH_CALLWNDPROC, hookFunction, hookModule, 0);
	setHookHandle(hookHandle);

	HANDLE threadHandle = CreateThread(nullptr, 0, ReadThread, nullptr, 0, nullptr);

	// Pump messages while we wait for the read thread to terminate.
	while (WaitForSingleObject(threadHandle, 100) == WAIT_TIMEOUT)
	{
		MSG message;
		while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}

	// We don't care about the results or if the calls succeed or fail.  We're trying our best to unload all the DLLs.
	UnhookWindowsHookEx(hookHandle);

	DWORD_PTR result;
	SendMessageTimeout(HWND_BROADCAST, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000, &result);

	CloseHandle(threadHandle);

	return 0;
}