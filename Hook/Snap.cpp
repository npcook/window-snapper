#include "Snap.h"

#include <Windows.h>
#include <algorithm>

#include "RectHelper.h"

Edge::Edge(int position, int start, int end) : Position(position), Start(start), End(end)
{ }

bool Edge::operator ==(const Edge& other) const
{
	return Position == other.Position && Start == other.Start && End == other.End;
}

bool Snap::WillHandleMessage(int message) const
{
	switch (message)
	{
	case WM_ENTERSIZEMOVE:
	case WM_EXITSIZEMOVE:
	case WM_MOVING:
	case WM_SIZING:
		return true;
	}
	return false;
}

bool Snap::HandleMessage(HWND window, int message, WPARAM wparam, LPARAM lparam)
{
	this->window = window;
	switch (message)
	{
	case WM_ENTERSIZEMOVE:
		return HandleEnterSizeMove();

	case WM_EXITSIZEMOVE:
		return HandleExitSizeMove();

	case WM_MOVING:
		return HandleMoving(*reinterpret_cast<RECT*>(lparam));

	case WM_SIZING:
		return HandleSizing(*reinterpret_cast<RECT*>(lparam), static_cast<int>(wparam));
	}

	return false;
}

bool Snap::HandleEnterSizeMove()
{
	for each (auto& edgeList in edges)
		edgeList.clear();

	// Pass "this" as the parameter
	EnumDisplayMonitors(nullptr, nullptr, [](HMONITOR monitorHandle, HDC, LPRECT, LPARAM param) -> BOOL
	{
		MONITORINFO monitorInfo;
		monitorInfo.cbSize = sizeof(monitorInfo);
		if (GetMonitorInfo(monitorHandle, &monitorInfo) == 0)
			return FALSE;

		// AddRectToEdges adds edges for the outside of a rectangle, which works well
		// for windows.  For monitors, however, we want to snap to the inside, so we 
		// swap the opposite edges.
		std::swap(monitorInfo.rcWork.left, monitorInfo.rcWork.right);
		std::swap(monitorInfo.rcWork.top, monitorInfo.rcWork.bottom);
		reinterpret_cast<Snap*>(param)->AddRectToEdges(monitorInfo.rcWork);

		return TRUE;
	}, (LPARAM) this);

#ifdef LOG
	HANDLE logFile = CreateFile(L"C:\\Users\\Nicholas\\Desktop\\log.txt", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif

	struct Param
	{
		Snap* _this;
		std::list<HRGN> windowRegions;
#ifdef LOG
		HANDLE logFile;
#endif
	};

	Param param;
	param._this = this;
#ifdef LOG
	param.logFile = logFile;
#endif

	EnumWindows([](HWND windowHandle, LPARAM _param) -> BOOL
	{
		// We don't want non-application windows or application windows the user can't see.
		auto param = reinterpret_cast<Param*>(_param);
		if (windowHandle == param->_this->window)
			return TRUE;
		if (!IsWindowVisible(windowHandle))
			return TRUE;
		if (IsIconic(windowHandle))
			return TRUE;

		int styles = (int) GetWindowLongPtr(windowHandle, GWL_STYLE);
		if ((styles & WS_CHILD) != 0 || (styles & WS_CAPTION) == 0)
			return TRUE;
		int extendedStyles = (int) GetWindowLongPtr(windowHandle, GWL_EXSTYLE);
		if ((extendedStyles & WS_EX_TOOLWINDOW) != 0 || (extendedStyles & WS_EX_NOACTIVATE) != 0)
			return TRUE;

		RECT thisRect;
		GetWindowRect(windowHandle, &thisRect);
		HRGN thisRegion = CreateRectRgnIndirect(&thisRect);

		// Since EnumWindows enumerates from the highest z-order to the lowest, we
		// can just check to see if this window is covered by any previous ones.
		bool isUserVisible = true;
		for each (HRGN region in param->windowRegions)
		{
			if (CombineRgn(thisRegion, thisRegion, region, RGN_DIFF) == NULLREGION)
			{
				isUserVisible = false;
				break;
			}
		}

		if (isUserVisible)
		{
			param->windowRegions.push_back(thisRegion);

			// Maximized windows by definition cover the whole work area, so the
			// only snap edges they would have are on the outside of the monitor.
			if (!IsZoomed(windowHandle))
				param->_this->AddRectToEdges(thisRect);

#ifdef LOG
			char title[256];
			GetWindowTextA(windowHandle, title, 256);
			char messageBuffer[256];
			sprintf_s(messageBuffer, "%s - RECT(%d, %d, %d, %d) - %s\r\n", title, thisRect.left, thisRect.top, thisRect.right, thisRect.bottom, !IsZoomed(windowHandle) ? "yes" : "no");

			DWORD bytesWritten;
			WriteFile(param->logFile, messageBuffer, strlen(messageBuffer), &bytesWritten, nullptr);
#endif
		}
		else
			DeleteObject(thisRegion);

		return TRUE;
	}, (LPARAM) &param);

	for each (HRGN region in param.windowRegions)
	{
		DeleteObject(region);
	}

	for each (auto& edgeList in edges)
	{
		edgeList.sort([](const Edge& _1, const Edge& _2) -> bool { return _1.Position < _2.Position; });
		edgeList.erase(std::unique(edgeList.begin(), edgeList.end()), edgeList.end());
	}

#ifdef LOG
	char messageBuffer[2048];
	strcpy_s(messageBuffer, "== BEGIN EDGES ==\r\n");

	for each (const auto& edgeList in edges)
	{
		for each (const Edge& edge in edgeList)
		{
			char edgeBuffer[64];
			sprintf_s(edgeBuffer, "%d %d %d\r\n", edge.Position, edge.Start, edge.End);
			strcat_s(messageBuffer, edgeBuffer);
		}
	}

	strcat_s(messageBuffer, "== END EDGES ==\r\n");

	DWORD bytesWritten;
	WriteFile(logFile, messageBuffer, strlen(messageBuffer), &bytesWritten, nullptr);
	CloseHandle(logFile);
#endif

	RECT bounds;
	GetWindowRect(window, &bounds);
	GetCursorPos(&originalCursorOffset);
	originalCursorOffset.x -= bounds.left;
	originalCursorOffset.y -= bounds.top;

	return true;
}

bool Snap::HandleExitSizeMove()
{
	inProgress = false;

	return true;
}

bool Snap::HandleMoving(RECT& bounds)
{
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

	for (int i = 0; i < Side::Count; ++i)
	{
		int snapPosition;
		if (CanSnapEdge(boundsEdges, (Side::Value) i, &snapPosition))
		{
			snapDirections[i] = true;
			snapEdges[i] = snapPosition;
		}
	}

	if ((GetKeyState(VK_MENU) & 0x8000) == 0 && (snapDirections[0] || snapDirections[1] || snapDirections[2] || snapDirections[3]))
	{
		if (!inProgress)
			inProgress = true;

		RECT snapRect = { snapEdges[0], snapEdges[1], snapEdges[2], snapEdges[3] };
		bounds = realBounds;
		SnapToRect(&bounds, snapRect, true, snapDirections[0], snapDirections[1], snapDirections[2], snapDirections[3]);

		return true;
	}
	else if (inProgress)
	{
		inProgress = false;
		bounds = realBounds;

		return true;
	}

	return false;
}

bool Snap::HandleSizing(RECT& bounds, int which)
{
	bool allowSnap[Side::Count] = { true, true, true, true };
	allowSnap[Side::Left] = (which == WMSZ_LEFT || which == WMSZ_TOPLEFT || which == WMSZ_BOTTOMLEFT);
	allowSnap[Side::Top] = (which == WMSZ_TOP || which == WMSZ_TOPLEFT || which == WMSZ_TOPRIGHT);
	allowSnap[Side::Right] = (which == WMSZ_RIGHT || which == WMSZ_TOPRIGHT || which == WMSZ_BOTTOMRIGHT);
	allowSnap[Side::Bottom] = (which == WMSZ_BOTTOM || which == WMSZ_BOTTOMLEFT || which == WMSZ_BOTTOMRIGHT);

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

	for (int i = 0; i < Side::Count; ++i)
	{
		if (!allowSnap[i])
			continue;
		int snapPosition;
		if (CanSnapEdge(boundsEdges, (Side::Value) i, &snapPosition))
		{
			snapDirections[i] = true;
			snapEdges[i] = snapPosition;
		}
	}

	if ((GetKeyState(VK_MENU) & 0x8000) == 0 && (snapDirections[0] || snapDirections[1] || snapDirections[2] || snapDirections[3]))
	{
		if (!inProgress)
			inProgress = true;

		RECT snapRect = { snapEdges[0], snapEdges[1], snapEdges[2], snapEdges[3] };
		SnapToRect(&bounds, snapRect, false, snapDirections[0], snapDirections[1], snapDirections[2], snapDirections[3]);

		return true;
	}
	else if (inProgress)
	{
		inProgress = false;

		return true;
	}

	return false;
}

// Breaks down a rect into 4 edges which are added to the global list of edges to snap to.
void Snap::AddRectToEdges(const RECT& rect)
{
	int startX = std::min<>(rect.left, rect.right);
	int endX = std::max<>(rect.left, rect.right);
	int startY = std::min<>(rect.top, rect.bottom);
	int endY = std::max<>(rect.top, rect.bottom);

	edges[Side::Left].push_front(Edge(rect.right, startY, endY));
	edges[Side::Right].push_front(Edge(rect.left, startY, endY));
	edges[Side::Top].push_front(Edge(rect.bottom, startX, endX));
	edges[Side::Bottom].push_front(Edge(rect.top, startX, endX));

	// Enabling this will cause the edges of the dragged window to snap to the similar edges of other windows.
	// For example, the top of the dragged window will snap to the top of another window.
#ifdef SNAP_TO_NEIGHBOR
	edges[Side::Left].push_front(Edge(rect.left, startY - 5, startY));
	edges[Side::Left].push_front(Edge(rect.left, endY, endY + 5));
	edges[Side::Right].push_front(Edge(rect.right, startY - 5, startY));
	edges[Side::Right].push_front(Edge(rect.right, endY, endY + 5));
	edges[Side::Top].push_front(Edge(rect.top, startX - 5, startX));
	edges[Side::Top].push_front(Edge(rect.top, endX, endX + 5));
	edges[Side::Bottom].push_front(Edge(rect.bottom, startX - 5, startX));
	edges[Side::Bottom].push_front(Edge(rect.bottom, endX, endX + 5));
#endif
}

void Snap::SnapToRect(RECT* bounds, const RECT& rect, bool retainSize, bool left, bool top, bool right, bool bottom) const
{
	if (left && right)
	{
		bounds->left = rect.left;
		bounds->right = rect.right;
	}
	else if (left)
	{
		if (retainSize)
			bounds->right += rect.left - bounds->left;
		bounds->left = rect.left;
	}
	else if (right)
	{
		if (retainSize)
			bounds->left += rect.right - bounds->right;
		bounds->right = rect.right;
	}
	else
	{
		bounds->left = bounds->left;
		bounds->right = bounds->right;
	}

	if (top && bottom)
	{
		bounds->top = rect.top;
		bounds->bottom = rect.bottom;
	}
	else if (top)
	{
		if (retainSize)
			bounds->bottom += rect.top - bounds->top;
		bounds->top = rect.top;
	}
	else if (bottom)
	{
		if (retainSize)
			bounds->top += rect.bottom - bounds->bottom;
		bounds->bottom = rect.bottom;
	}
	else
	{
		bounds->top = bounds->top;
		bounds->bottom = bounds->bottom;
	}
}

bool Snap::CanSnapEdge(int boundsEdges[Side::Count], Side::Value which, int* snapPosition) const
{
	for each (const Edge& edge in edges[which])
	{
		int edgeDistance = edge.Position - boundsEdges[which];
		// Since each edge list is sorted, if the snap edge is past our bound's edge,
		// we know that none of the other edges will work.
		if (edgeDistance >= snapDistance)
			break;
		else if (edgeDistance > -snapDistance)
		{
			// The modulos get the position of the edges perpendicular to the bound
			// edge we are working on.
			int min = std::min<>(boundsEdges[(which + 1) % Side::Count], boundsEdges[(which + 3) % Side::Count]);
			int max = std::max<>(boundsEdges[(which + 1) % Side::Count], boundsEdges[(which + 3) % Side::Count]);
			if (max > edge.Start && min < edge.End)
			{
				*snapPosition = edge.Position;
				return true;
			}
		}
	}

	return false;
}