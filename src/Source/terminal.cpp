#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <Mmsystem.h>
#include <commctrl.h>

#include "Python.h"

#include "terminal.h"
#include "curses_win32.h"
#include "defines.h"
#include "globals.h"
#include "lines.h"

#define _TRACING
#include "trace.h"

/* void Cls_OnSizing(HWND hwnd, UINT edge, RECT *size) */
//#define HANDLE_WM_SIZING(hwnd, wParam, lParam, fn) \
//    (BOOL)((fn)((hwnd), (UINT)(wParam), (RECT *)(lParam)))

PyTypeObject curses_terminalType = 
{
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	const_cast<char*>(TerminalProp),	/*tp_name*/
	sizeof(Terminal),	/*tp_basicsize*/
	0,				/*tp_itemsize*/
	
	/* methods */
	(destructor)Terminal::dealloc, 	/*tp_dealloc*/
	0,						/*tp_print*/
	0, /*tp_getattr*/
	0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
	0,			/* tp_call */
	0,			/* tp_str */
	0,			/* tp_getattro */
	0,			/* tp_setattro */
	0,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	"Terminal Object",		/* tp_doc */
	0,		/* tp_traverse */
	0,		/* tp_clear */
	0,		/* tp_richcompare */
	0,		/* tp_weaklistoffset */
	0,		/* tp_iter */
	0,		/* tp_iternext */
	0, /* tp_methods */
	0, /* tp_members */
    0,                      /*tp_getset*/
    0,                      /*tp_base*/
    0,                      /*tp_dict*/
    0,                      /*tp_descr_get*/
    0,                      /*tp_descr_set*/
    0,                      /*tp_dictoffset*/
    0,                      /*tp_init*/
    0,                      /*tp_alloc*/
    0,                      /*tp_new*/
    0,                      /*tp_free*/
    0,                      /*tp_is_gc*/
};

const char *Terminal::ClassName = "WCurses Terminal";

void Terminal::CursorPos(int x, int y)
{
	TRACE("Terminal_CursorPos");

	this->x = x;
	this->y = y;
}

void Terminal::UpdateCursorPos()
{
	TRACE("Terminal_UpdateCursorPos");

	if (this->has_caret)
	{
		SetCaretPos(
			this->x * this->cell_width,
			(this->y * this->cell_height) + this->caret_yoffs
			);
	}
}

void Terminal::dealloc(Terminal *self)
{
	TRACE("Terminal_dealloc");

	FreeLines(self->buffer, self->height);
	DestroyWindow(self->win);

	Py_DECREF(self->keybuffer);

	PyObject_Del(self);
}

Terminal *Terminal::New()
{
	HFONT font;
	HMENU sysmenu;
	MENUITEMINFO mitem;
	Terminal *newterm;

	TRACE("Terminal_New");

	newterm = PyObject_NEW(Terminal, &curses_terminalType);
	if (newterm == NULL)
		return NULL;


	newterm->height = DEFAULT_HEIGHT;
	newterm->width = DEFAULT_WIDTH;

	newterm->keybuffer = PyList_New(0);
	newterm->ungetch = -1;
	newterm->waiting_for_key = 0;

	newterm->buffer = AllocLines(newterm->height, newterm->width, SPACE);

	newterm->has_caret = 0;

	// Cursor to top left
	newterm->x = 0;
	newterm->y = 0;

	// IO options
	// What should these default to? The defaults should probably be set via corresponding functions.
	newterm->_cbreak = 0;
	newterm->_echo = 1;
	newterm->_raw = 0;
	newterm->_qiflush = 0;
	newterm->allow8bitInput = 1;
	newterm->_keypad = 0;
	newterm->_halfdelay = 0;

	// Create window
	newterm->win = CreateWindow (
		Terminal::ClassName,         // window class name
		Terminal::ClassName,     // window caption
	    WS_OVERLAPPEDWINDOW,     // window style
	    CW_USEDEFAULT,           // initial x position
	    CW_USEDEFAULT,           // initial y position
	    CW_USEDEFAULT,           // initial x size
	    CW_USEDEFAULT,           // initial y size
	    NULL,                    // parent window handle
	    NULL,                    // window menu handle
	    g_dll_instance,               // program instance handle
		NULL) ;		             // creation parameters

	// Hook up the Win32 window for this terminal to the associated Terimnal structure
	SetProp(newterm->win, TerminalProp, newterm);

	// Create statusbar
	newterm->status_bar = CreateStatusWindow(
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM | SBARS_SIZEGRIP,
		Version,
		newterm->win,
		CID_STATUSBAR
		);

	// Add some items to the system menu
	sysmenu = GetSystemMenu(newterm->win, FALSE);

	mitem.cbSize = sizeof(MENUITEMINFO);
	mitem.fMask = MIIM_TYPE;
	mitem.fType = MFT_SEPARATOR;
	mitem.dwItemData = 0;

	InsertMenuItem(sysmenu, -1, TRUE, &mitem);

	// Set the default terminal font
	font=CreateFontIndirect(&g_default_font);
	newterm->SetFont(font);

	newterm->SetCursorVisibility(1);

	return newterm;
}

void Terminal::RedrawText()
{
	RECT r;

	TRACE("Terminal_RedrawText");

	r.left = 0;
	r.top = 0;
	r.right = this->pixel_width - 1;
	// Don't invalidate the status bar
	r.bottom = this->pixel_height - 1 - this->status_bar_height;

	InvalidateRect(this->win,&r,0);
	UpdateWindow(this->win);
}

void Terminal::Show()
{
	TRACE("Terminal_Show");
	ShowWindow(this->win, SW_SHOWNORMAL);
	UpdateWindow(this->win);
}

// Must be done AFTER a font is selected.
int Terminal::SetCursorVisibility(int visibility)
{
	TRACE("Terminal_SetCursorVisibility");
	
	int old_visibility = this->cursor_visibility;
	this->cursor_visibility = visibility;

	switch(this->cursor_visibility)
	{
	case 0:
		this->caret_yoffs = 0;
		if (this->has_caret) HideCaret(this->win);
		break;

	case 1:
		this->caret_height = 3;
		this->caret_yoffs = this->cell_height - this->caret_height;
		break;

	case 2:
		this->caret_height = this->cell_height;
		this->caret_yoffs = 0;
		break;
	}

	return old_visibility;
}

void Terminal::SetFont(HFONT newfont)
{
	HDC DC;
	TEXTMETRIC Metrics;
	RECT rect,cli_rect;
	int xh,xw;

	RECT rect_sbar;

	TRACE("Terminal_SetFont");

	DC=GetDC(this->win);
	SelectFont(DC, newfont);
	GetTextMetrics(DC, &Metrics);
	ReleaseDC(this->win, DC);

	this->font = newfont;

	this->cell_width = Metrics.tmAveCharWidth;
	this->cell_height = Metrics.tmHeight + Metrics.tmExternalLeading;
	
	// find the minimum window size
	GetWindowRect(this->win,&rect);
	GetClientRect(this->win,&cli_rect);

	GetWindowRect(this->status_bar, &rect_sbar);
	this->status_bar_height = rect_sbar.bottom - rect_sbar.top + 1;

	xw=rect.right-rect.left+1-cli_rect.right;
	xh=rect.bottom-rect.top+1-cli_rect.bottom;
	
	this->pixel_width=xw + (this->cell_width * this->width);
	this->pixel_height=xh + (this->cell_height * this->height) + this->status_bar_height;

	MoveWindow(this->win, rect.left, rect.top, this->pixel_width, this->pixel_height, 1);
}

void Terminal::OnPaint()
{
	int y, x;
	int colors, fg, bg;
	int r,g,b;
	int bg_rgb, fg_rgb;
	char ch;
	char_cell ccch;
	HDC	hdc;
	PAINTSTRUCT ps;
//	int last_attr;

	hdc = BeginPaint(this->win, &ps);
	SelectFont(hdc,this->font);
	
	for (y=0; y<this->height; y++)
	{
		x = 0;
		while (x<this->width)
		{
			ccch = this->buffer[y][x];
			colors = g_color_pairs[ (ccch & ATTR_COLOR) >> 8 ];
			fg = colors & 0xf;
			bg = (colors & 0xf0) >> 4;

			crack_color(fg, &r, &g, &b);

			if (ccch & ATTR_BOLD)
			{
				r = r?255:0;
				g = g?255:0;
				b = b?255:0;
			}

			fg_rgb = RGB(r, g, b);

			crack_color(bg, &r, &g, &b);
			bg_rgb = RGB(r, g, b);

			if (! (ccch & ATTR_REVERSE))
			{
				SetTextColor(hdc,fg_rgb);
				SetBkColor(hdc,bg_rgb);
			}
			else
			{
				SetTextColor(hdc,bg_rgb);
				SetBkColor(hdc,fg_rgb);
			}
/*
			// Try to find a run of characters with the same attributes.
			last_attr = ccch & ~ATTR_CHAR;
			
			while ((x < this->width) && ((this->buffer[y][x] & ~ATTR_CHAR) == last_attr))
			{
				ch = this->buffer[y][x] & ATTR_CHAR;
				TextOut(hdc, x * this->cell_width, y * this->cell_height, &ch, 1);
				x++;
			}
*/
			ch = this->buffer[y][x] & ATTR_CHAR;
			TextOut(hdc, x * this->cell_width, y * this->cell_height, &ch, 1);
			x++;
		}
	}

	EndPaint(this->win, &ps);
	this->UpdateCursorPos();
}

void Terminal::OnSetFocus(HWND oldFocus)
{
	if (this->CursorVisibility())
	{
		CreateCaret(this->win, NULL, this->cell_width, this->caret_height);
		this->UpdateCursorPos();
		ShowCaret(this->win);
		this->has_caret = 1;
	}
}

void Terminal::OnKillFocus(HWND newFocus)
{
	if (this->has_caret)
	{
//				HideCaret(term->win);
		DestroyCaret();
		this->has_caret = 0;
	}
}

void Terminal::OnChar(TCHAR ch, int cRepeat)
{
	for (int i=0; i < cRepeat; i++)
	{
		PyList_Append(this->keybuffer, PyInt_FromLong(ch));
	}
}

void Terminal::OnKey(UINT vkey, BOOL fDown, int cRepeat, UINT flags)
{
	PyObject *mappedkey;
	int i;

	// Don't do anything for KEYUP messages.
	if (!fDown)	return;

	mappedkey = PyDict_GetItem(g_keymap_dict, PyInt_FromLong(vkey));
	if(mappedkey)
	{
		for (i=0; i < cRepeat; i++)
		{
			Py_INCREF(mappedkey);
			PyList_Append(this->keybuffer, mappedkey);
		}
	}
}

void Terminal::OnSize(UINT state, int cx, int cy)
{
	MoveWindow(this->StatusBar(), 0, 0, this->pixel_width, this->StatusBarHeight(), TRUE);
}

BOOL Terminal::OnSizing(UINT edge, RECT *size)
{
	if (size->right-size->left+1 < this->pixel_width)
		size->right = size->left+this->pixel_width;

	if (size->bottom-size->top+1 < this->pixel_height)
		size->bottom=size->top+this->pixel_height;

	return TRUE;
}

LRESULT CALLBACK Terminal::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Terminal *term = (Terminal *)GetProp(hwnd, TerminalProp);

	switch (uMsg)
	{
	case WM_PAINT:
		return term->OnPaint(), 0L;

	case WM_SETFOCUS:
		return term->OnSetFocus((HWND)wParam), 0L;

	case WM_KILLFOCUS:
		return term->OnKillFocus((HWND)wParam), 0L;

	case WM_CHAR:
		return term->OnChar((TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L;

	case WM_KEYDOWN:
		return term->OnKey((UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L;

	case WM_SIZE:
		return term->OnSize((UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L;

	case WM_SIZING:
		return term->OnSizing((UINT)(wParam), (RECT *)(lParam));

	case WM_NCDESTROY:
		RemoveProp(hwnd, TerminalProp);
		return 0;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
