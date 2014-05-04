#include "RectHelper.h"

#include <Windows.h>

bool operator ==(const RECT& _1, const RECT& _2)
{
	return _1.left == _2.left && _1.top == _2.top && _1.right == _2.right && _1.bottom == _2.bottom;
}

bool operator !=(const RECT& _1, const RECT& _2)
{
	return !(_1 == _2);
}