#include <Windows.h>
#include <string>
#include <list>
#include <algorithm>

#pragma data_seg(".shared")

HHOOK hookHandle;

#pragma data_seg()

struct Side
{
	enum Value
	{
		Left = 0,
		Top = 1,
		Right = 2,
		Bottom = 3,

		Count = 4,
	};
};

struct Edge
{
	int Position;
	int Start;
	int End;

	Edge(int position, int start, int end) : Position(position), Start(start), End(end)
	{ }

	bool operator ==(const Edge& other) const
	{
		return Position == other.Position && Start == other.Start && End == other.End;
	}
};

WNDPROC oldSubclassProc;
HANDLE logHandle;
bool enabled;
POINT originalCursorOffset = { 0 };
bool inProgress = false;
std::list<Edge> edges[4];

void AddRectToEdges(const RECT& rect)
{
	int startX = std::min<>(rect.left, rect.right);
	int endX = std::max<>(rect.left, rect.right);
	int startY = std::min<>(rect.top, rect.bottom);
	int endY = std::max<>(rect.top, rect.bottom);

	edges[Side::Left].push_front(Edge(rect.right, startY, endY));
	edges[Side::Right].push_front(Edge(rect.left, startY, endY));
	edges[Side::Top].push_front(Edge(rect.bottom, startX, endX));
	edges[Side::Bottom].push_front(Edge(rect.top, startX, endX));

	edges[Side::Left].push_front(Edge(rect.left, startY - 5, startY));
	edges[Side::Left].push_front(Edge(rect.left, endY, endY + 5));
	edges[Side::Right].push_front(Edge(rect.right, startY - 5, startY));
	edges[Side::Right].push_front(Edge(rect.right, endY, endY + 5));
	edges[Side::Top].push_front(Edge(rect.top, startX - 5, startX));
	edges[Side::Top].push_front(Edge(rect.top, endX, endX + 5));
	edges[Side::Bottom].push_front(Edge(rect.bottom, startX - 5, startX));
	edges[Side::Bottom].push_front(Edge(rect.bottom, endX, endX + 5));
}

bool operator ==(const RECT& _1, const RECT& _2)
{
	return _1.left == _2.left && _1.top == _2.top && _1.right == _2.right && _1.bottom == _2.bottom;
}

bool operator !=(const RECT& _1, const RECT& _2)
{
	return !(_1 == _2);
}

void SnapBounds(RECT* bounds, const RECT& edges, bool retainSize, bool left, bool top, bool right, bool bottom)
{
	if (left && right)
	{
		bounds->left = edges.left;
		bounds->right = edges.right;
	}
	else if (left)
	{
		if (retainSize)
			bounds->right += edges.left - bounds->left;
		bounds->left = edges.left;
	}
	else if (right)
	{
		if (retainSize)
			bounds->left += edges.right - bounds->right;
		bounds->right = edges.right;
	}
	else
	{
		bounds->left = bounds->left;
		bounds->right = bounds->right;
	}

	if (top && bottom)
	{
		bounds->top = edges.top;
		bounds->bottom = edges.bottom;
	}
	else if (top)
	{
		if (retainSize)
			bounds->bottom += edges.top - bounds->top;
		bounds->top = edges.top;
	}
	else if (bottom)
	{
		if (retainSize)
			bounds->top += edges.bottom - bounds->bottom;
		bounds->bottom = edges.bottom;
	}
	else
	{
		bounds->top = bounds->top;
		bounds->bottom = bounds->bottom;
	}
}

LRESULT CALLBACK TemporarySubclassProc(HWND window, int message, WPARAM wparam, LPARAM lparam)
{
	SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR) oldSubclassProc);

	if (message == WM_ENTERSIZEMOVE)
	{
		for each (auto& edgeList in edges)
			edgeList.clear();

		EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR monitorHandle, HDC, LPRECT, LPARAM) -> BOOL
		{
			MONITORINFO monitorInfo;
			monitorInfo.cbSize = sizeof(monitorInfo);
			if (GetMonitorInfo(monitorHandle, &monitorInfo) == 0)
				return FALSE;

			std::swap(monitorInfo.rcWork.left, monitorInfo.rcWork.right);
			std::swap(monitorInfo.rcWork.top, monitorInfo.rcWork.bottom);
			AddRectToEdges(monitorInfo.rcWork);

			return TRUE;
		}, 0);

		struct Param
		{
			HWND thisWindow;
			std::list<RECT> windowRects;
		};

		Param param;
		param.thisWindow = window;

		EnumWindows([](HWND windowHandle, LPARAM _param) -> BOOL
		{
			auto param = reinterpret_cast<Param*>(_param);
			if (windowHandle == param->thisWindow)
				return TRUE;
			if (!IsWindowVisible(windowHandle))
				return TRUE;
			if (IsIconic(windowHandle))
				return TRUE;
			wchar_t title[256];
			GetWindowText(windowHandle, title, 256);

			int styles = (int) GetWindowLongPtr(windowHandle, GWL_STYLE);
			if ((styles & WS_CHILD) != 0)
				return TRUE;
			int extendedStyles = (int) GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
			if ((extendedStyles & WS_EX_TOOLWINDOW) != 0)
				return TRUE;
			WINDOWPLACEMENT windowPlacement;
			windowPlacement.length = sizeof(windowPlacement);
			GetWindowPlacement(windowHandle, &windowPlacement);
			bool isMaximized = (windowPlacement.showCmd == SW_SHOWMAXIMIZED);

			RECT thisRect;
			GetWindowRect(windowHandle, &thisRect);

			bool isUserVisible = true;
			for each (RECT rect in param->windowRects)
			{
				RECT intersection;
				if (IntersectRect(&intersection, &thisRect, &rect) != 0)
				{
					if (intersection == thisRect)
					{
						isUserVisible = false;
						break;
					}
				}
			}

			if (isUserVisible)
			{
				param->windowRects.push_back(thisRect);
				if (!isMaximized)
					AddRectToEdges(thisRect);
			}

			return TRUE;
		}, (LPARAM) &param);

		for each (auto& edgeList in edges)
		{
			edgeList.sort([](const Edge& _1, const Edge& _2) -> bool { return _1.Position < _2.Position; });
			edgeList.erase(std::unique(edgeList.begin(), edgeList.end()), edgeList.end());
		}
	}
	else if (message == WM_MOVING)
	{
		RECT& bounds = *reinterpret_cast<RECT*>(lparam);

		// The difference between the cursor position and the top-left corner of the dragged window.
		// This is normally constant while dragging a window, but when near an edge that we snap to,
		// this changes.
		POINT cursorOffset;
		GetCursorPos(&cursorOffset);
		cursorOffset.x -= bounds.left;
		cursorOffset.y -= bounds.top;

		// While we are snapping a window, the window displayed to the user is not the "real" location
		// of the window, i.e. where it would be if we hadn't snapped it.
		RECT realBounds;
		if (inProgress)
		{
			POINT offsetDiff = { cursorOffset.x - originalCursorOffset.x, cursorOffset.y - originalCursorOffset.y };
			SetRect(&realBounds, bounds.left + offsetDiff.x, bounds.top + offsetDiff.y,
					bounds.right + offsetDiff.x, bounds.bottom + offsetDiff.y);
		}
		else
			realBounds = bounds;

		int boundsEdges[Side::Count] = { realBounds.left, realBounds.top, realBounds.right, realBounds.bottom };
		int snapEdges[Side::Count] = { SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX };
		bool snapDirections[Side::Count] = { false, false, false, false };

		// Snap a window when it is within snapDistance units of a screen edge.
		const int snapDistance = 15;

		for (int i = 0; i < Side::Count; ++i)
		{
			for each (const Edge& edge in edges[i])
			{
				int edgeDistance = edge.Position - boundsEdges[i];
				if (edgeDistance >= snapDistance)
					break;
				else if (edgeDistance > -snapDistance)
				{
					int min = std::min<>(boundsEdges[(i + 1) % Side::Count], boundsEdges[(i + 3) % Side::Count]);
					int max = std::max<>(boundsEdges[(i + 1) % Side::Count], boundsEdges[(i + 3) % Side::Count]);
					if (max > edge.Start && min < edge.End)
					{
						snapDirections[i] = true;
						snapEdges[i] = edge.Position;
						break;
					}
				}
			}
		}

		if ((GetKeyState(VK_MENU) & 0x8000) == 0 && (snapDirections[0] || snapDirections[1] || snapDirections[2] || snapDirections[3]))
		{
			if (!inProgress)
			{
				inProgress = true;
				originalCursorOffset = cursorOffset;
			}

			RECT snapRect = { snapEdges[0], snapEdges[1], snapEdges[2], snapEdges[3] };
			bounds = realBounds;
			SnapBounds(&bounds, snapRect, true, snapDirections[0], snapDirections[1], snapDirections[2], snapDirections[3]);

			return 1;
		}
		else if (inProgress)
		{
			inProgress = false;
			bounds = realBounds;

			return 1;
		}

		return 0;
	}
	else if (message == WM_SIZING)
	{
		bool allowSnap[Side::Count] = { true, true, true, true };
		allowSnap[Side::Left] = (wparam == WMSZ_LEFT || wparam == WMSZ_TOPLEFT || wparam == WMSZ_BOTTOMLEFT);
		allowSnap[Side::Top] = (wparam == WMSZ_TOP || wparam == WMSZ_TOPLEFT || wparam == WMSZ_TOPRIGHT);
		allowSnap[Side::Right] = (wparam == WMSZ_RIGHT || wparam == WMSZ_TOPRIGHT || wparam == WMSZ_BOTTOMRIGHT);
		allowSnap[Side::Bottom] = (wparam == WMSZ_BOTTOM || wparam == WMSZ_BOTTOMLEFT || wparam == WMSZ_BOTTOMRIGHT);

		RECT& bounds = *reinterpret_cast<RECT*>(lparam);

		// The difference between the cursor position and the top-left corner of the dragged window.
		// This is normally constant while dragging a window, but when near an edge that we snap to,
		// this changes.
		POINT cursorOffset;
		GetCursorPos(&cursorOffset);
		cursorOffset.x -= bounds.left;
		cursorOffset.y -= bounds.top;

		int boundsEdges[Side::Count] = { bounds.left, bounds.top, bounds.right, bounds.bottom };
		int snapEdges[Side::Count] = { SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX };
		bool snapDirections[Side::Count] = { false, false, false, false };

		// Snap a window when it is within snapDistance units of a screen edge.
		const int snapDistance = 15;

		for (int i = 0; i < Side::Count; ++i)
		{
			if (!allowSnap[i])
				continue;
			for each (const Edge& edge in edges[i])
			{
				int edgeDistance = edge.Position - boundsEdges[i];
				if (edgeDistance >= snapDistance)
					break;
				else if (edgeDistance > -snapDistance)
				{
					int min = std::min<>(boundsEdges[(i + 1) % Side::Count], boundsEdges[(i + 3) % Side::Count]);
					int max = std::max<>(boundsEdges[(i + 1) % Side::Count], boundsEdges[(i + 3) % Side::Count]);
					if (max > edge.Start && min < edge.End)
					{
						snapDirections[i] = true;
						snapEdges[i] = edge.Position;
						break;
					}
				}
			}
		}

		if ((GetKeyState(VK_MENU) & 0x8000) == 0 && (snapDirections[0] || snapDirections[1] || snapDirections[2] || snapDirections[3]))
		{
			if (!inProgress)
			{
				inProgress = true;
				originalCursorOffset = cursorOffset;
			}

			RECT snapRect = { snapEdges[0], snapEdges[1], snapEdges[2], snapEdges[3] };
			SnapBounds(&bounds, snapRect, false, snapDirections[0], snapDirections[1], snapDirections[2], snapDirections[3]);

			return 1;
		}
		else if (inProgress)
		{
			inProgress = false;

			return 1;
		}

		return 0;
	}
	else if (message == WM_EXITSIZEMOVE)
	{
		inProgress = false;

		return 0;
	}

Passthrough:
	return CallWindowProc(oldSubclassProc, window, message, wparam, lparam);
}

extern "C"
{
	LRESULT CALLBACK HookProc(int code, WPARAM wparam, LPARAM lparam)
	{
		auto cwp = reinterpret_cast<CWPSTRUCT*>(lparam);

		if (enabled && code == HC_ACTION)
		{
			if (cwp->message == WM_SIZING || cwp->message == WM_MOVING || cwp->message == WM_ENTERSIZEMOVE || cwp->message == WM_EXITSIZEMOVE)
			{
				oldSubclassProc = (WNDPROC) SetWindowLongPtr(cwp->hwnd, GWLP_WNDPROC, (LONG_PTR) TemporarySubclassProc);
			}

			return 0;
		}
		else if (code < 0)
			return CallNextHookEx(hookHandle, code, wparam, lparam);
		else
			return CallNextHookEx(hookHandle, code, wparam, lparam);
	}

	void SetHookHandle(HHOOK handle)
	{
		hookHandle = handle;
	}
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
	{
//		wchar_t fileName[MAX_PATH];
//		GetModuleFileName(nullptr, fileName, MAX_PATH);
//		if (wcsstr(fileName, L"notepad.exe") != nullptr)
//		{
			enabled = true;
//			logHandle = CreateFile(L"C:\\Users\\Nicholas\\Desktop\\log.txt", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
//			SetFilePointer(logHandle, 0, nullptr, FILE_END);
//		}
//		else
//			enabled = false;
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		CloseHandle(logHandle);
		break;
	}
	}
	return TRUE;
}