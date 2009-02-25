#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <Mmsystem.h>
#include <commctrl.h>

#include "resource.h"

#include "Python.h"
#include "Terminal.h"
#include "curses_win32.h"
#include "defines.h"
#include "globals.h"

#include "Trace.h"

HINSTANCE g_dll_instance;
WNDCLASSEX  g_terminal_wndclass;

const char *TerminalLongName = "WCurses Terminal / Win32";
const char *TerminalProp = "terminal";
const char *Version = "WCurses Alpha 3";

void InitializeDefaultFont();
long GetScreenFontHeight(int PointSize);

// Default terminal font. Some values set in InitializeDefaultFont
LOGFONT g_default_font =
{
	0,	// height
	0,	//width
	0,0,	// escapement, orientation
	0,	// weight
	0,	// italic
	0,	// underline
	0,	// strikeout
	OEM_CHARSET,	// Character set
	0,0,0,0,
	"Courier New"	// typeface
};


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD wDataSeg, LPVOID lpReserved)
{
	INITCOMMONCONTROLSEX ccex;

	switch(wDataSeg)
	{
    	case DLL_PROCESS_ATTACH:
			TRACE("Loading _WCurses.dll...");

    		g_dll_instance=hInst;

			ccex.dwSize = sizeof(ccex);
			ccex.dwICC = ICC_WIN95_CLASSES;
			InitCommonControlsEx(&ccex);

			RegisterTerminalWindowClass();
			InitializeDefaultFont();
			InitializeColors();

			g_called_initscr = 0;
			g_default_term = NULL;
			g_current_term = NULL;
	      	return 1;
		break;
	  		
		case DLL_PROCESS_DETACH:
			TRACE("Detaching _WCurses.dll...");

			UnregisterClass(Terminal::ClassName, g_dll_instance);
			Py_DECREF(g_keymap_dict);

			// We grabbed an extra reference to the default terminal when initscr() was called,
			// release it here.
			if(g_called_initscr)
				Py_XDECREF(g_default_term);

			return 1;
		break;
	}

	return 1;
}

void RegisterTerminalWindowClass()
{
	g_terminal_wndclass.cbSize = sizeof (g_terminal_wndclass);
	g_terminal_wndclass.style = CS_HREDRAW | CS_VREDRAW;
	g_terminal_wndclass.lpfnWndProc = Terminal::WindowProc;
	g_terminal_wndclass.cbClsExtra = 0;
	g_terminal_wndclass.cbWndExtra = 0;
	g_terminal_wndclass.hInstance = g_dll_instance;
	g_terminal_wndclass.hCursor = 0;
	g_terminal_wndclass.hbrBackground = GetStockBrush(BLACK_BRUSH);
	g_terminal_wndclass.lpszMenuName = NULL;
	g_terminal_wndclass.lpszClassName = Terminal::ClassName;
	g_terminal_wndclass.hIcon = LoadIcon (g_dll_instance,MAKEINTRESOURCE(IDI_ICONAT));
	g_terminal_wndclass.hIconSm = LoadIcon (g_dll_instance,MAKEINTRESOURCE(IDI_ICONAT));

	RegisterClassEx (&g_terminal_wndclass) ;
}

void ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void InitializeDefaultFont()
{
	g_default_font.lfHeight = GetScreenFontHeight(10);
}

long GetScreenFontHeight(int PointSize)
{
	HDC dc = GetDC(NULL);
	long fontheight = -MulDiv(PointSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
	ReleaseDC(NULL, dc);
	return fontheight;
}
