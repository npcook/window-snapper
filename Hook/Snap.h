#pragma once

#include <list>
#include <Windows.h>

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

	Edge(int position, int start, int end);

	bool operator ==(const Edge& other) const;
};

class Snap
{
	// A sorted list of edges which can be snapped to for each side of the window.
	std::list<Edge> edges[Side::Count];
	// The difference between the cursor position and the top-left of the window being dragged.
	POINT originalCursorOffset;
	// Is the window currently being snapped?
	bool inProgress = false;
	// The window handle
	HWND window;

	void AddRectToEdges(const RECT& rect);
	void SnapToRect(RECT* bounds, const RECT& rect, bool retainSize, bool left, bool top, bool right, bool bottom);

	bool HandleEnterSizeMove();
	bool HandleExitSizeMove();
	bool HandleMoving(RECT& bounds);
	bool HandleSizing(RECT& bounds, int which);
public:
	bool WillHandleMessage(int message) const;
	bool HandleMessage(HWND window, int message, WPARAM wparam, LPARAM lparam);
};