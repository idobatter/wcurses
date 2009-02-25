#pragma once
#ifndef _curses_terminal_h
#define _curses_terminal_h
/*
	Struct for a single Terminal window. Implemented as a Win32 GUI window.

	These objects are returned by newterm, and are made active by set_term.
	delterm is implemented as the object destructor, so "del MyTerm" will free the structure.

	If initscr() is called instead of (or before) newterm(), a Terminal is created 
	and pointed to by g_default_terminal. 
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "char_cell.h"

struct Terminal
{
public:
	PyObject_HEAD

	static const char *ClassName;

	// Windows window handle
	HWND win;
	HDC backbuffer;

	short has_caret;	// Has the Win32 caret.

	// Terminal size
	int height;
	int width;

	// "Hardware" cursor (caret) position
	int x;
	int y;
	int caret_yoffs;
	int caret_height;

	// Window size, in pixels, includes non-client area.
	int pixel_width;
	int pixel_height;

	// The screen buffer. Pointers to height lines of size width.
	char_cell **buffer;

	// Text font
	HFONT font;	// Current terminal font
	int cell_height;	// Font cell size
	int cell_width;

	// Keyboard buffer
	PyObject *keybuffer;
	int ungetch;

	int waiting_for_key;

	// I/O Options
	int _cbreak;
	int _echo;
	int _raw;
	int _qiflush;
	int _keypad;
	int _halfdelay;

	int allow8bitInput;

	// Methods
	static Terminal *New();
	static void dealloc(Terminal *self);

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnPaint();
	void OnSetFocus(HWND oldFocus);
	void OnKillFocus(HWND newFocus);
	void OnChar(TCHAR ch, int cRepeat);
	void OnKey(UINT vkey, BOOL fDown, int cRepeat, UINT flags);
	void OnSize(UINT state, int cx, int cy);
	BOOL OnSizing(UINT edge, RECT *size);

	int CursorVisibility() const { return this->cursor_visibility; };
	HWND StatusBar() const { return this->status_bar; };
	int StatusBarHeight() const { return this->status_bar_height; };

	void CursorPos(int x, int y);
	void RedrawText();
	int SetCursorVisibility(int visibility);
	void SetFont(HFONT newfont);
	void Show();
	void UpdateCursorPos();

private:
	static const int DEFAULT_HEIGHT=25;
	static const int DEFAULT_WIDTH=80;
	static const int CID_STATUSBAR=2;

	int cursor_visibility;
	HWND status_bar;	
	int status_bar_height;
};

extern Terminal *g_default_term;
extern Terminal *g_current_term;

extern PyTypeObject curses_terminalType;

#endif
