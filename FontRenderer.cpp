/*
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "stdafx.h"
#include "FontRenderer.h"

#define SCREEN_DX 240
#define SCREEN_DY 400

MemoryScreen memScr;

class FontDlg
{
	LOGFONTA lf;

public:
	CHOOSEFONTA cf;

	FontDlg(HWND hWnd, LONG h);
	const LOGFONTA* getLogFont() const
	{
		return &lf;
	}
};

FontDlg::FontDlg(HWND hWnd, LONG h)
{
	memset(&lf, 0, sizeof(lf));
	memset(&cf, 0, sizeof(cf));
	cf.lStructSize = sizeof(cf);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_INITTOLOGFONTSTRUCT;

	lf.lfHeight = h;
}

struct Rect : public RECT
{
	Rect(int x, int y, int dx, int dy)
	{
		top = y;
		left = x;
		bottom = y + dy;
		right = x + dx;
	}
};

void MemoryScreen::init(HWND hWnd)
{
	HDC hdc = GetDC(hWnd);
	hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmp = CreateCompatibleBitmap(hdcMem, SCREEN_DX, SCREEN_DY); // B&W
	SelectObject(hdcMem, hbmp);

	Rect r(0, 0, SCREEN_DX, SCREEN_DY);
	FillRect(hdcMem, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));

	ReleaseDC(hWnd, hdc);
}

//#include "upng.h"

void draw(HDC hdc)
{
	BitBlt(hdc, 0, 0, SCREEN_DX, SCREEN_DY, memScr.hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdc, GetStockObject(BLACK_PEN));
	for (int i = 1; i < 5; ++i)
	{
		MoveToEx(hdc, 0, i * 20, NULL);
		LineTo(hdc, SCREEN_DX, i * 20);
	}
	MoveToEx(hdc, 0, SCREEN_DY, NULL);
	LineTo(hdc, SCREEN_DX, SCREEN_DY);
	LineTo(hdc, SCREEN_DX, 0);
/*
	static const unsigned char gray4[] =
	{
#include "gray4.txt"
	};

	upng_t* upng = upng_new_from_bytes(gray4, sizeof(gray4));
	if (upng != NULL)
	{
		upng_decode(upng);
		if (upng_get_error(upng) == UPNG_EOK)
		{
//			struct Color3 { BYTE r, g, b; };
			const BYTE* p = upng_get_buffer(upng);
			for (size_t x = 0; x < upng_get_width(upng); ++x)
				for (size_t y = 0; y < upng_get_height(upng); ++y)
				{
					BYTE b = p[x / 2 + y * upng_get_width(upng) / 2];
					if (0 == (x & 1))
						b >>= 4;
					SetPixel(hdc, 400 + x, y, RGB(((b & 8) >> 3) * 255, ((b & 4) >> 2) * 255, ((b & 2)>>1) * 255));
				}
		}
		upng_free(upng);
	}
*/
}

static int charHeight(HDC hdc, WCHAR c)
{
	RECT r;
	if (DrawTextW(hdc, &c, 1, &r, DT_CALCRECT))
		return (int)(r.bottom - r.top);
	return 0;
}

static int charWidth(HDC hdc, WCHAR c)
{
	RECT r = { 0 };
	if (DrawTextW(hdc, &c, 1, &r, DT_CALCRECT | DT_NOPREFIX))
		if (r.right > r.left)
			return (int)(r.right - r.left);
	return 0;
}

static int maxCharWidth(HDC hdc, WCHAR firstChar, WCHAR sz)
{
	LONG maxW = 0;
	for (WCHAR i = firstChar; i < firstChar + sz; ++i)
	{
		int w = charWidth(hdc, i);
		if (w > maxW)
			maxW = w;
	}
	return (int)maxW;
}

static void outputOneChar(HDC hdc, WCHAR c, int h, int bytesH, int w, FILE* out)
{
	Rect rChr(0, 0, w, h);
	FillRect(memScr.hdcMem, &rChr, (HBRUSH)GetStockObject(WHITE_BRUSH));
	TextOutW(memScr.hdcMem, 0, 0, &c, 1);

	fprintf(out, "\n%u, ", charWidth(memScr.hdcMem, c));
	for (int x = 0; x < w; ++x)
	{
		unsigned char n = 0;
		int b = bytesH;
		for (int y = 0; y < h; ++y)
		{
			COLORREF pix = GetPixel(hdc, x, y);
			if (0 == pix) // black on white
				n |= (1 << (y % 8));
			if (7 == y % 8)
			{
				fprintf(out, n ? " 0x%X," : " %u,", n);
				n = 0;
				b--;
			}
		}
		if (b--)
			fprintf(out, n ? " 0x%X," : " %u,", n);
		ASSERT_DBG(b == 0);
	}
}

static void outputStruct(FILE* out, int h, int w, int numBlocks)
{
	fprintf(out, "#ifndef __DEVFONT_H__\n");
	fprintf(out, "#define __DEVFONT_H__\n\n");

	fprintf(out, "typedef struct\n{\n");
	fprintf(out, "unsigned int base, sz;\n");
	fprintf(out, "const unsigned char* sym;\n} DevFontBlock;\n\n");

	fprintf(out, "typedef struct\n{\n");
	fprintf(out, "unsigned char h, bytesH, maxW;\n");
	fprintf(out, "DevFontBlock block[%u];\n} DevFont;\n\n", numBlocks);

	fprintf(out, "#endif\n");
}

static void outputBlockData(FILE* out, HDC hdc, WCHAR blockFrom, WCHAR blockSz, int h, int bytesH, int w)
{
	fprintf(out, "static const unsigned char fontBlock%u_%u[%u] = \n{", blockFrom, blockSz, blockSz * (w * bytesH + 1));
	for (WCHAR i = 0; i < blockSz; ++i)
	{
		outputOneChar(hdc, blockFrom + i, h, bytesH, w, out);
	}
	fprintf(out, "\n};\n\n");
}

struct OneBlockRange
{
	WCHAR base, sz;
};

void outputDefinition(FILE* out, int h, int bytesH, int w, size_t numBlocks, const OneBlockRange* ranges)
{
	fprintf(out, "#include \"devfont.h\"\n\n");
	fprintf(out, "const DevFont font%ux%u = \n{ %u, %u, %u,\n{", h, w, h, bytesH, w);
	for (size_t i = 0; i < numBlocks; ++i)
	{
		fprintf(out, "{ %u, %u, fontBlock%u_%u },\n", ranges[i].base, ranges[i].sz, ranges[i].base, ranges[i].sz);
	}
	fprintf(out, "}};\n");
}

static void outputRasterFont(HDC hdc)
{
	static const OneBlockRange ranges[] =
	{
		{ 32, 126 - 32 }, { 0x410, 64 },
	};

	FILE* out = fopen("devfont.h", "wb");

	int maxW = 0;
	const size_t numBlocks = sizeof(ranges) / sizeof(*ranges);
	ASSERT_DBG(numBlocks);
	for (size_t i = 0; i < numBlocks; ++i)
	{
		int w = maxCharWidth(hdc, ranges[i].base, ranges[i].sz);
		if (w > maxW)
			maxW = w;
	}
	int h = charHeight(hdc, ranges[0].base);

	outputStruct(out, h, maxW, numBlocks);

	fclose(out);

	char s[64];
	sprintf(s, "devfont%ux%u.c", h, maxW);
	FILE* outC = fopen(s, "wb");

	unsigned char bytesH = h / 8;
	if (h > bytesH * 8)
		bytesH++;
	for (size_t i = 0; i < numBlocks; ++i)
	{
		outputBlockData(out, memScr.hdcMem, ranges[i].base, ranges[i].sz, h, bytesH, maxW);
	}

	outputDefinition(outC, h, bytesH, maxW, numBlocks, ranges);
	fclose(outC);
}

void chooseFont(HWND hWnd)
{
	FontDlg dlg(hWnd, -16);
	if (ChooseFontA(&dlg.cf))
	{
		HFONT hf = CreateFontIndirectA(dlg.getLogFont());
		SelectObject(memScr.hdcMem, hf);

		outputRasterFont(memScr.hdcMem);

		// test 
		Rect rScr(0, 0, SCREEN_DX, SCREEN_DY);
		FillRect(memScr.hdcMem, &rScr, (HBRUSH)GetStockObject(WHITE_BRUSH));
		const WCHAR s[] = { '1', '2', '3', 0x410, 0x411, 0x412, 0x413, 0x414, 0x410 + 25, 'H', 'e', 'l', 'l', 'o', };
		TextOutW(memScr.hdcMem, 0, 0, s, sizeof(s) / sizeof(*s));

		SelectObject(memScr.hdcMem, GetStockObject(SYSTEM_FONT));
		DeleteObject(hf);

		InvalidateRect(hWnd, NULL, FALSE);
	}
}

//#include "devfont.h"
//#include "devfont21x15.c"