#include <Windows.h>
#include <cmath>
#include <string>
#include <list>
#include <algorithm>
#include <array>

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

// Get the work area of the monitor a rectangle is in
bool GetMonitorRectFromRect(const RECT& rect, RECT* monitorRect)
{
	HMONITOR monitorHandle = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
	if (monitorHandle == nullptr)	// Should never happen
		return false;

	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	if (GetMonitorInfo(monitorHandle, &monitorInfo) == 0)
		return false;

	*monitorRect = monitorInfo.rcWork;
	return true;
}

bool AreRectsEqual(const RECT& _1, const RECT& _2)
{
	return _1.left == _2.left && _1.top == _2.top && _1.right == _2.right && _1.bottom == _2.bottom;
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
		RECT& bounds = *reinterpret_cast<RECT*>(lParam);
		_bounds = bounds;
		RECT monitor;
		if (!GetMonitorRectFromRect(bounds, &monitor))
			break;

		// The difference between the cursor position and the top-left corner of the dragged window.
		// This is normally constant while dragging a window, but when near an edge that we snap to,
		// this changes.
		cursorOffset;
		GetCursorPos(&cursorOffset);
		cursorOffset.x -= bounds.left;
		cursorOffset.y -= bounds.top;

		// While we are snapping a window, the window displayed to the user is not the "real" location
		// of the window, i.e. where it would be if we hadn't snapped it.
//		RECT realBounds;
		if (inProgress)
		{
			POINT offsetDiff = { cursorOffset.x - originalCursorOffset.x, cursorOffset.y - originalCursorOffset.y };
			SetRect(&realBounds, bounds.left + offsetDiff.x, bounds.top + offsetDiff.y,
			        bounds.right + offsetDiff.x, bounds.bottom + offsetDiff.y);
		}
		else
			realBounds = bounds;

		int boundsEdges[4] = { realBounds.left, realBounds.top, realBounds.right, realBounds.bottom };
		int snapEdges[4] = { SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX };
		bool snapDirections[4] = { false, false, false, false };

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
					int min = std::min<>(boundsEdges[(i + 1) % 4], boundsEdges[(i + 3) % 4]);
					int max = std::max<>(boundsEdges[(i + 1) % 4], boundsEdges[(i + 3) % 4]);
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

		return DefWindowProc(window, message, wParam, lParam);
	}

	case WM_SIZING:
	{
		bool allowSnap[4] = { true, true, true, true };
		allowSnap[Side::Left] = (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT);
		allowSnap[Side::Top] = (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT);
		allowSnap[Side::Right] = (wParam == WMSZ_RIGHT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_BOTTOMRIGHT);
		allowSnap[Side::Bottom] = (wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT);

		lastwParam = wParam;
		InvalidateRect(window, nullptr, TRUE);
		RECT& bounds = *reinterpret_cast<RECT*>(lParam);
		_bounds = bounds;
		RECT monitor;
		if (!GetMonitorRectFromRect(bounds, &monitor))
			break;

		// The difference between the cursor position and the top-left corner of the dragged window.
		// This is normally constant while dragging a window, but when near an edge that we snap to,
		// this changes.
		cursorOffset;
		GetCursorPos(&cursorOffset);
		cursorOffset.x -= bounds.left;
		cursorOffset.y -= bounds.top;

		int boundsEdges[4] = { bounds.left, bounds.top, bounds.right, bounds.bottom };
		int snapEdges[4] = { SHRT_MIN, SHRT_MIN, SHRT_MAX, SHRT_MAX };
		bool snapDirections[4] = { false, false, false, false };

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
					int min = std::min<>(boundsEdges[(i + 1) % 4], boundsEdges[(i + 3) % 4]);
					int max = std::max<>(boundsEdges[(i + 1) % 4], boundsEdges[(i + 3) % 4]);
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

		return DefWindowProc(window, message, wParam, lParam);
	}

	case WM_ENTERSIZEMOVE:
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

		break;
	}

	case WM_EXITSIZEMOVE:
		inProgress = false;
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