#pragma once

class MemoryScreen
{
public:
	HDC hdcMem;

	void init(HWND hWnd);
};

extern MemoryScreen memScr;

void draw(HDC hdc);
void chooseFont(HWND hWnd);
