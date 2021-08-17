#pragma once
#ifndef UNICODE
#error encoding must be set as Unicode
#endif

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <iostream>
#include <chrono>
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <condition_variable>

enum PixelType
{
	Solid = 0x2588,
	OneQuater = 0x2591,
	TwoQuaters = 0x2592,
	ThreeQuaters = 0x2593
};

enum Color
{
	FG_Black		= 0x0000,
	FG_DarkBlue		= 0x0001,
	FG_DarkGreen	= 0x0002,
	FG_DarkCyan		= 0x0003,
	FG_DarkRed		= 0x0004,
	FG_DarkMagenta	= 0x0005,
	FG_DarkYellow	= 0x0006,
	FG_Grey			= 0x0007,
	FG_DarkGrey		= 0x0008,
	FG_Blue			= 0x0009,
	FG_Green		= 0x000A,
	FG_Cyan			= 0x000B,
	FG_Red			= 0x000C,
	FG_Magenta		= 0x000D,
	FG_Yellow		= 0x000E,
	FG_White		= 0x000F,

	BG_Black		= 0x0000,
	BG_DarkBlue		= 0x0010,
	BG_DarkGreen	= 0x0020,
	BG_DarkCyan		= 0x0030,
	BG_DarkRed		= 0x0040,
	BG_DarkMagenta	= 0x0050,
	BG_DarkYellow	= 0x0060,
	BG_Grey			= 0x0070,
	BG_DarkGrey		= 0x0080,
	BG_Blue			= 0x0090,
	BG_Green		= 0x00A0,
	BG_Cyan			= 0x00B0,
	BG_Red			= 0x00C0,
	BG_Magenta		= 0x00D0,
	BG_Yellow		= 0x00E0,
	BG_White		= 0x00F0,
};

class Sprite
{
public:
	Sprite() = default;
	Sprite(int w, int h)
	{
		Create(w, h);
	}

private:
	int width = 0;
	int height = 0;

	short* glyphs = nullptr;
	short* colors = nullptr;

	void Create(int w, int h)
	{
		width = w;
		height = h;
		glyphs = new short[w * h];
		colors = new short[w * h];
		for (int i = 0; i < w * h; ++i)
		{
			glyphs[i] = L' ';
			colors[i] = FG_Black;
		}
	}

public:
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }

	short GetGlyph(int x, int y) const
	{
		if (x < 0 || x >= width || y < 0 || y >= height)
			return L' ';
		else
			return glyphs[x + y * width];
	}

	short GetColor(int x, int y) const
	{
		if (x < 0 || x >= width || y < 0 || y >= height)
			return FG_Black;
		else
			return colors[x + y * width];
	}

	short SampleGlyph(float x, float y) const
	{
		int sx = (int)(x * (float)width);
		int sy = (int)(y * (float)height - 1.0f);
		if (sx < 0 || sx >= width || sy < 0 || sy >= height)
			return L' ';
		else
			return glyphs[sx + sy * width];
	}

	short SampleColor(float x, float y) const
	{
		int sx = (int)(x * (float)width);
		int sy = (int)(y * (float)height - 1.0f);
		if (sx < 0 || sx >= width || sy < 0 || sy >= height)
			return FG_Black;
		else
			return colors[sx + sy * width];
	}

	void SetGlyph(int x, int y, short c)
	{
		if (x < 0 || x >= width || y < 0 || y >= height)
			return;
		else
			glyphs[x + y * width] = c;
	}

	void SetColor(int x, int y, short c)
	{
		if (x < 0 || x >= width || y < 0 || y >= height)
			return;
		else
			colors[x + y * width] = c;
	}

	bool Save(const std::wstring& filename)
	{
		FILE* file = nullptr;
		_wfopen_s(&file, filename.c_str(), L"wb");
		if (file == nullptr)
			return false;

		fwrite(&width, sizeof(int), 1, file);
		fwrite(&height, sizeof(int), 1, file);
		fwrite(colors, sizeof(short), width * height, file);
		fwrite(glyphs, sizeof(short), width * height, file);

		fclose(file);

		return true;
	}

	bool Load(const std::wstring& filename)
	{
		delete[] glyphs;
		delete[] colors;
		width = 0;
		height = 0;

		FILE* file = nullptr;
		_wfopen_s(&file, filename.c_str(), L"rb");
		if (file == nullptr)
			return false;

		fread(&width, sizeof(int), 1, file);
		fread(&height, sizeof(int), 1, file);

		Create(width, height);

		fread(glyphs, sizeof(short), width * height, file);
		fread(colors, sizeof(short), width * height, file);

		fclose(file);
		return true;
	}
};

class Console69
{
public:
	Console69()
		:
		screenWidth{ 80 }, screenHeight{ 30 },
		console{ GetStdHandle(STD_OUTPUT_HANDLE) },
		consoleIn{ GetStdHandle(STD_INPUT_HANDLE) },
		mousePosX{ 0 }, mousePosY{ 0 },
		appName{ L"Default" },
		audioEnabled{ false }
	{
		std::memset(keyNewState, 0, 256 * sizeof(short));
		std::memset(keyOldState, 0, 256 * sizeof(short));
		std::memset(keys, 0, 256 * sizeof(KeyState));
	}

	~Console69()
	{
		SetConsoleActiveScreenBuffer(originalConsole);
		delete[] screenBuffer;
	}

	Console69(const Console69&) = delete;
	Console69& operator=(const Console69&) = delete;


public:
	void EnableAudio()
	{
		audioEnabled = true;
	}

	int Initialize(int screenw, int screenh, int fontw, int fonth)
	{
		if (console == INVALID_HANDLE_VALUE)
			return Error(L"Invalid Handle");

		screenWidth = screenw;
		screenHeight = screenh;

		// Windows BS, must set screen buffer first, then font size
		// to prevent weird behaviors

		consoleWindow = { 0, 0, 1, 1 };
		SetConsoleWindowInfo(console, TRUE, &consoleWindow);

		COORD coord = { (short)screenWidth, (short)screenHeight };
		if (!SetConsoleScreenBufferSize(console, coord))
			return Error(L"SetConsoleScreenBufferSize");

		if(!SetConsoleActiveScreenBuffer(console))
			return Error(L"SetConsoleActiveScreenBuffer");

		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize.X = fontw;
		cfi.dwFontSize.Y = fonth;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

		wcscpy_s(cfi.FaceName, L"Consolas");
		if (!SetCurrentConsoleFontEx(console, false, &cfi))
			return Error(L"SetCurrentConsoleFontEx");

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!GetConsoleScreenBufferInfo(console, &csbi))
			return Error(L"GetConsoleScreenBufferInfo");
		if (screenWidth > csbi.dwMaximumWindowSize.X)
			return Error(L"Screen Width / Font Width too big");
		if(screenHeight > csbi.dwMaximumWindowSize.Y)
			return Error(L"Screen Height / Font Height Too Big");

		// set console window size
		consoleWindow = { 0, 0, (short)(screenWidth - 1), (short)(screenHeight - 1) };
		if (!SetConsoleWindowInfo(console, TRUE, &consoleWindow))
			return Error(L"SetConsoleWindowInfo");

		if (!SetConsoleMode(consoleIn, ENABLE_EXTENDED_FLAGS |
			ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
			return Error(L"SetConsoleMode");

		// memory
		screenBuffer = new CHAR_INFO[screenWidth * screenHeight];
		memset(screenBuffer, 0, sizeof(CHAR_INFO) * screenWidth * screenHeight);

		SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);
		return 1;
	}

	virtual void Draw(int x, int y, short cha = 0x2588, short col = 0x000F)
	{
		if (x >= 0 && x < screenWidth && y >= 0 && y < screenHeight)
		{
			screenBuffer[x + y * screenWidth].Char.UnicodeChar = cha;
			screenBuffer[x + y * screenWidth].Attributes = col;
		}
	}

	void Fill(int x1, int y1, int x2, int y2, short cha = 0x2588, short col = 0x000F)
	{
		Clip(x1, y1);
		Clip(x2, y2);
		for (int x = x1; x < x2; ++x)
			for (int y = y1; y < y2; ++y)
				Draw(x, y, cha, col);
	}

	void Clip(int& x, int& y)
	{
		if (x < 0)
			x = 0;
		if (x >= screenWidth)
			x = screenWidth;
		if (y < 0)
			y = 0;
		if (y >= screenHeight)
			y = screenHeight;
	}

	void DrawString(int x, int y, const std::wstring& str, short col = 0x000F)
	{
		for (size_t i = 0; i < str.size(); ++i)
		{
			screenBuffer[x + y * screenWidth + i].Char.UnicodeChar = str[i];
			screenBuffer[x + y * screenHeight + i].Attributes = col;
		}
	}

	void DrawStringAlpha(int x, int y, const std::wstring& str, short col = 0x000F)
	{
		for (size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] != L' ')
			{
				screenBuffer[x + y * screenWidth + i].Char.UnicodeChar = str[i];
				screenBuffer[x + y * screenWidth + i].Attributes = col;
			}
		}
	}

	void DrawLine(int x1, int y1, int x2, int y2, short cha = 0x2588, short col = 0x000F)
	{
		int x{}, y{}, dx{}, dy{}, dx1{}, dy1{}, px{}, py{}, xe{}, ye{};
		int i{};
		dx = x2 - x1;
		dy = y2 - y1;
		dx1 = abs(dx);
		dy1 = abs(dy);
		px = dy1 * 2 - dx1;
		py = dx1 * 2 - dy1;
		if (dx1 >= dy1)
		{
			if (dx >= 0)
			{
				x = x1;
				y = y1;
				xe = x2;
			}
			else
			{
				x = x2;
				y = y2;
				xe = x1;
			}

			Draw(x, y, cha, col);

			for (int i = 0; x < xe; ++i)
			{
				x = x + 1;
				if (px < 0)
				{
					px += 2 * dy1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
						y += 1;
					else
						y -= 1;
					px += 2 * (dy1 - dx1);
				}
				Draw(x, y, cha, col);
			}
		}
		else
		{
			if (dy >= 0)
			{
				x = x1;
				y = y1;
				ye = y2;
			}
			else
			{
				x = x2;
				y = y2;
				ye = y1;
			}
			Draw(x, y, cha, col);

			for (i = 0; y < ye; ++i)
			{
				y += 1;
				if (py <= 0)
				{
					py += 2 * dx1;
				}
				else
				{
					if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
						x += 1;
					else
						x -= 1;
					py += 2 * (dx1 - dy1);
				}
				Draw(x, y, cha, col);
			}
		}
	}

	void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short cha = 0x2588, short col = 0x000F)
	{
		DrawLine(x1, y1, x2, y2, cha, col);
		DrawLine(x2, y2, x3, y3, cha, col);
		DrawLine(x3, y3, x1, y1, cha, col);
	}

	// https://www.avrfreaks.net/sites/default/files/triangles.c
	void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, short cha = 0x2588, short col = 0x000F)
	{
		auto SWAP = [](int& x, int& y) { int t = x; x = y; y = t; };
		auto drawline = [&](int sx, int ex, int ny) { for (int i = sx; i <= ex; i++) Draw(i, ny, cha, col); };

		int t1x, t2x, y, minx, maxx, t1xp, t2xp;
		bool changed1 = false;
		bool changed2 = false;
		int signx1, signx2, dx1, dy1, dx2, dy2;
		int e1, e2;
		// sort vertices
		if (y1 > y2) { SWAP(y1, y2); SWAP(x1, x2); }
		if (y1 > y3) { SWAP(y1, y3); SWAP(x1, x3); }
		if (y2 > y3) { SWAP(y2, y3); SWAP(x2, x3); }

		t1x = t2x = x1; y = y1;   // starting points
		dx1 = (int)(x2 - x1); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
		else signx1 = 1;
		dy1 = (int)(y2 - y1);

		dx2 = (int)(x3 - x1); if (dx2 < 0) { dx2 = -dx2; signx2 = -1; }
		else signx2 = 1;
		dy2 = (int)(y3 - y1);

		if (dy1 > dx1) {   // swap values
			SWAP(dx1, dy1);
			changed1 = true;
		}
		if (dy2 > dx2) {   // swap values
			SWAP(dy2, dx2);
			changed2 = true;
		}

		e2 = (int)(dx2 >> 1);
		// flat top, just process the second half
		if (y1 == y2) goto next;
		e1 = (int)(dx1 >> 1);

		for (int i = 0; i < dx1;) {
			t1xp = 0; t2xp = 0;
			if (t1x < t2x) { minx = t1x; maxx = t2x; }
			else { minx = t2x; maxx = t1x; }
			// process first line until y value is about to change
			while (i < dx1) {
				i++;
				e1 += dy1;
				while (e1 >= dx1) {
					e1 -= dx1;
					if (changed1) t1xp = signx1;//t1x += signx1;
					else          goto next1;
				}
				if (changed1) break;
				else t1x += signx1;
			}
			// move line
		next1:
			// process second line until y value is about to change
			while (1) {
				e2 += dy2;
				while (e2 >= dx2) {
					e2 -= dx2;
					if (changed2) t2xp = signx2;//t2x += signx2;
					else          goto next2;
				}
				if (changed2)     break;
				else              t2x += signx2;
			}
		next2:
			if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
			if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
			drawline(minx, maxx, y);    // draw line from min to max points found on the y
										 // now increase y
			if (!changed1) t1x += signx1;
			t1x += t1xp;
			if (!changed2) t2x += signx2;
			t2x += t2xp;
			y += 1;
			if (y == y2) break;

		}
	next:
		// second half
		dx1 = (int)(x3 - x2); if (dx1 < 0) { dx1 = -dx1; signx1 = -1; }
		else signx1 = 1;
		dy1 = (int)(y3 - y2);
		t1x = x2;

		if (dy1 > dx1) {   // swap values
			SWAP(dy1, dx1);
			changed1 = true;
		}
		else changed1 = false;

		e1 = (int)(dx1 >> 1);

		for (int i = 0; i <= dx1; i++) {
			t1xp = 0; t2xp = 0;
			if (t1x < t2x) { minx = t1x; maxx = t2x; }
			else { minx = t2x; maxx = t1x; }
			// process first line until y value is about to change
			while (i < dx1) {
				e1 += dy1;
				while (e1 >= dx1) {
					e1 -= dx1;
					if (changed1) { t1xp = signx1; break; }//t1x += signx1;
					else          goto next3;
				}
				if (changed1) break;
				else   	   	  t1x += signx1;
				if (i < dx1) i++;
			}
		next3:
			// process second line until y value is about to change
			while (t2x != x3) {
				e2 += dy2;
				while (e2 >= dx2) {
					e2 -= dx2;
					if (changed2) t2xp = signx2;
					else          goto next4;
				}
				if (changed2)     break;
				else              t2x += signx2;
			}
		next4:

			if (minx > t1x) minx = t1x; if (minx > t2x) minx = t2x;
			if (maxx < t1x) maxx = t1x; if (maxx < t2x) maxx = t2x;
			drawline(minx, maxx, y);
			if (!changed1) t1x += signx1;
			t1x += t1xp;
			if (!changed2) t2x += signx2;
			t2x += t2xp;
			y += 1;
			if (y > y3) return;
		}
	}

	void DrawCircle(int xc, int yc, int r, short cha = 0x2588, short col = 0x000F)
	{
		int x{};
		int y = r;
		int p = 3 - 2 * r;
		if (!r)
			return;

		while (y >= x)
		{
			Draw(xc - x, yc - y, cha, col);	//upper left left
			Draw(xc - y, yc - x, cha, col);	//upper upper left
			Draw(xc + y, yc - x, cha, col);	//upper upper right
			Draw(xc + x, yc - y, cha, col);	//upper right right
			Draw(xc - x, yc + y, cha, col);	//lower left left
			Draw(xc - y, yc + x, cha, col);	//lower lower left
			Draw(xc + y, yc + x, cha, col);	//lower lower right
			Draw(xc + x, yc + y, cha, col);	//lower right right
			if (p < 0)
				p += 4 * x++ + 6;
			else
				p += 4 * (x++ - y--) + 10;
		}
	}

	// research according to wikipedia Kappa
	void FillCircle(int xc, int yc, int r, short cha = 0x2588, short col = 0x000F)
	{
		int x = 0;
		int y = r;
		int p = 3 - 2 * r;
		if (!r)
			return;

		auto drawline = [&](int sx, int ex, int ny)
		{
			for (int i = sx; i <= ex; ++i)
				Draw(i, ny, cha, col);
		};

		while (y >= x)
		{
			// draw scan-lines instead
			drawline(xc - x, xc + x, yc - y);
			drawline(xc - y, xc + y, yc - x);
			drawline(xc - x, xc + x, yc + y);
			drawline(xc - y, xc + y, yc + x);
			if (p < 0)
				p += 4 * x++ + 6;
			else
				p += 4 * (x++ - y--) + 10;
		}
	}

	void DrawSprite(int x, int y, const Sprite& sprite)
	{
		for (int i = 0; i < sprite.GetWidth(); ++i)
		{
			for (int j = 0; j < sprite.GetHeight(); ++j)
			{
				if (sprite.GetGlyph(i, j) != L' ')
					Draw(x + i, y + j, sprite.GetGlyph(i, j), sprite.GetColor(i, j));
			}
		}
	}

	void DrawSpritePartial(int x, int y, const Sprite& sprite, int ox, int oy, int w, int h)
	{
		for (int i = 0; i < w; ++i)
		{
			for (int j = 0; j < h; ++j)
			{
				if (sprite.GetGlyph(ox + i, oy + j) != L' ')
					Draw(x + i, y + j, sprite.GetGlyph(ox + i, oy + j), sprite.GetColor(ox + i, oy + j));
			}
		}
	}

	void DrawWireFrame(const std::vector<std::pair<float, float>>& coordinates,
		float x, float y, float r = 0.0f, float s = 1.0f, short col = FG_White, short cha = Solid)
	{
		std::vector<std::pair<float, float>> transformed;
		int vertices = coordinates.size();
		transformed.resize(vertices);

		// rotate
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].first = coordinates[i].first * cosf(r) - coordinates[i].second * sinf(r);
			transformed[i].second = coordinates[i].first * sinf(r) + coordinates[i].second * cosf(r);
		}

		// scale
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].first = coordinates[i].first * s;
			transformed[i].second = coordinates[i].second * s;
		}

		// translate
		for (int i = 0; i < vertices; ++i)
		{
			transformed[i].first = coordinates[i].first + x;
			transformed[i].second = coordinates[i].second + y;
		}

		// draw polygon
		for (int i = 0; i < vertices; ++i)
		{
			int j = i + 1;
			DrawLine(
				(int)transformed[i % vertices].first, (int)transformed[i % vertices].second,
				(int)transformed[j % vertices].first, (int)transformed[j % vertices].second,
				cha, col
			);
		}
	}


public:
	void Start()
	{
		atomActive = true;
		std::thread thread = std::thread(&Console69::GameThread, this);

		thread.join();
	}

	int GetScreenWidth() const { return screenWidth; }
	int GetScreenHeight() const { return screenHeight; }

public:
	virtual bool OnAwake() = 0;
	virtual bool OnUpdate(float deltaTime) = 0;

	virtual bool OnDestroy() { return true; }


private:
	void GameThread()
	{
		if (!OnAwake())
			atomActive = false;

		auto timepoint1 = std::chrono::system_clock::now();
		auto timepoint2 = std::chrono::system_clock::now();

		while (atomActive)
		{
			while (atomActive)
			{
				timepoint2 = std::chrono::system_clock::now();
				std::chrono::duration<float> elapsed = timepoint2 - timepoint1;
				timepoint1 = timepoint2;
				float elapsedTime = elapsed.count();

				// keyboard
				for (int i = 0; i < 256; ++i)
				{
					keyNewState[i] = GetAsyncKeyState(i);
					keys[i].Press = false;
					keys[i].Release = false;

					if (keyNewState[i] != keyOldState[i])
					{
						if (keyNewState[i] & 0x8000)
						{
							keys[i].Press = !keys[i].Hold;
							keys[i].Hold = true;
						}
						else
						{
							keys[i].Release = true;
							keys[i].Hold = false;
						}
					}
					keyOldState[i] = keyNewState[i];
				}

				// mouse
				INPUT_RECORD inputBuffer[32]{};
				DWORD events{};
				GetNumberOfConsoleInputEvents(console, &events);
				if (events > 0)
					ReadConsoleInput(consoleIn, inputBuffer, events, &events);

				// only deal with mouse events
				for (DWORD i = 0; i < events; ++i)
				{
					switch (inputBuffer[i].EventType)
					{
						case FOCUS_EVENT:
						{
							consoleFocused = inputBuffer[i].Event.FocusEvent.bSetFocus;
						}
						break;

						case MOUSE_EVENT:
						{
							switch (inputBuffer[i].Event.MouseEvent.dwEventFlags)
							{
								case MOUSE_MOVED:
								{
									mousePosX = inputBuffer[i].Event.MouseEvent.dwMousePosition.X;
									mousePosY = inputBuffer[i].Event.MouseEvent.dwMousePosition.Y;
								}
								break;

								case 0:
								{
									for (int m = 0; m < 5; ++m)
										mouseNewState[m] = (inputBuffer[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
								}
								break;

								default:
									break;
							}
						}
						break;

						default:
							break;
					}
				}

				for (int m = 0; m < 5; ++m)
				{
					mouse[m].Press = false;
					mouse[m].Release = false;

					if (mouseNewState[m] != mouseOldState[m])
					{
						if (mouseNewState[m])
						{
							mouse[m].Press = true;
							mouse[m].Hold = true;
						}
						else
						{
							mouse[m].Release = true;
							mouse[m].Hold = false;
						}
					}

					mouseOldState[m] = mouseNewState[m];
				}

				if (!OnUpdate(elapsedTime))
					atomActive = false;

				// display status
				wchar_t title[256];
				swprintf_s(title, 256, L"Console69 %s FPS: %3.2f", appName.c_str(), 1.0f / elapsedTime);
				SetConsoleTitle(title);
				WriteConsoleOutput(
					console, screenBuffer, 
					{ (short)screenWidth, (short)screenHeight },
					{ 0,0 }, &consoleWindow
				);
			}

			if (OnDestroy())
			{
				delete[] screenBuffer;
				SetConsoleActiveScreenBuffer(originalConsole);
				gameEnded.notify_one();
			}
			else
			{
				atomActive = true;
			}
		}
	}

protected:
	struct KeyState
	{
		bool Press;
		bool Release;
		bool Hold;
	} keys[256], mouse[5];

	int mousePosX;
	int mousePosY;

public:
	KeyState GetKey(int keycode) { return keys[keycode]; }
	int GetMouseX() { return mousePosX; }
	int GetMouseY() { return mousePosY; }
	KeyState GetMouse(int button) { return mouse[button]; }
	bool IsFocused() { return consoleFocused; }

protected:
	int screenWidth;
	int screenHeight;
	CHAR_INFO* screenBuffer;
	std::wstring appName;
	HANDLE originalConsole;
	CONSOLE_SCREEN_BUFFER_INFO originalConsoleInfo;
	HANDLE console;
	HANDLE consoleIn;
	SMALL_RECT consoleWindow;

	short keyOldState[256] = { 0 };
	short keyNewState[256] = { 0 };
	bool mouseOldState[5] = { 0 };
	bool mouseNewState[5] = { 0 };
	bool consoleFocused = true;
	bool audioEnabled = false;

protected:
	int Error(const wchar_t* msg)
	{
		wchar_t buffer[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, 256, NULL);
		SetConsoleActiveScreenBuffer(originalConsole);
		wprintf(L"ERROR: %s\n\t%s\n", msg, buffer);
		return 0;
	}

	static BOOL CloseHandler(DWORD evt)
	{
		// called in a seperate OS thread
		// so make sure before OnDestroy()
		if (evt == CTRL_CLOSE_EVENT)
		{
			atomActive = false;

			std::unique_lock<std::mutex> lock(mux);
			gameEnded.wait(lock);
		}
		return true;
	}

	static std::atomic<bool> atomActive;
	static std::condition_variable gameEnded;
	static std::mutex mux;
};

std::atomic<bool> Console69::atomActive{ false };
std::condition_variable Console69::gameEnded;
std::mutex Console69::mux;