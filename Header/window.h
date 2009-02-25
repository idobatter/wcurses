#pragma once
#ifndef _curses_window_h
#define _curses_window_h

#include "Python.h"

struct Terminal;

struct Window
{
	PyObject_HEAD

	// Which terminal is this window attached to?
	Terminal *term;

	// This window's parent, or NULL if it's top-level
	Window *parent;
	bool isPad;

	int offX,offY;		// offset x and y from parent window or screen

	// Character buffer
	char_cell **buffer;
	int height,width;

	char_cell attr; // Default attributes used for writes
	char_cell background; // Background character

	char_cell backgd;

	int curx,cury;				// current cursor position for this window.
	
	short isScrolling;
	short noDelay;
	int _leave_cursor;	// If true, don't update the terminal's hardware cursor

	void AdvanceCursor();
};
//typedef struct Window Window;

extern PyTypeObject curses_windowType;
#define Window_Check(v)	 ((v)->ob_type == &curses_windowType)

PyObject *Window_New(Window *parent, int begin_x, int begin_y, int width, int height, bool flags);
PyObject *Window_addch(Window *self,PyObject *args);
PyObject *Window_refresh(Window *self, PyObject *args);

void Window_AdvanceCursor(Window *win);
void Window_ScrollUp(Window *win, int topline, int botline, int lines);
void Window_ScrollDown(Window *win, int topline, int botline, int lines);

extern PyMethodDef Window_Methods[];

#endif
