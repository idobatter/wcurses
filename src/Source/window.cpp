#include "Python.h"
#include "pyhelper.h"

#include "curses.h"
#include "window.h"
#include "terminal.h"
#include "curses_win32.h"
#include "defines.h"
#include "globals.h"
#include "lines.h"

#define _TRACING
#include "trace.h"

#include "acs.h"

int Window_ContainsPoint(Window *win, int x, int y)
{
	TRACE("Window_ContainsPoint");
	return (win->offX <= x) && 
		(x < win->offX + win->width) && 
		(win->offY <= y) && 
		(y < win->offY + win->height);
}

PyObject *Window_enclose(Window *self, PyObject *args)
{
	TRACE("Window_enclose");

	int y,x;
	if (!PyArg_ParseTuple(args, "ii", &y, &x))
		return NULL;

	return PyInt_FromLong(Window_ContainsPoint(self, x, y));
}

int Window_SetCur(Window *win,int x,int y)
{
	TRACE("Window_SetCur");

	if (x>=0 && x<win->width && 
		y>=0 && y<win->height	)
	{
		win->curx=x;
		win->cury=y;

		win->term->CursorPos(win->offX + win->curx, win->offY + win->cury);
		return 1;
	}

	return 0;	// error...
}

int Window_AddChar(Window *win, char_cell ch, int advance_cursor)
{
	// we always assume that the cursor is in a valid position...
  	win->buffer[win->cury][win->curx]=ch;

	if (advance_cursor)
		win->AdvanceCursor();

  	return 1;
}

/*
	Advances the cursor once to the right for the given window,
	wrapping to the next line and scrolling if needed.
*/
void Window::AdvanceCursor()
{
  	this->curx++;
  	
  	if (this->curx < this->width)
  		return;
  		
	this->curx=0;
	this->cury++;
	
	if (this->cury < this->height)
		return;

	// if we went past the bottom right character then either scroll
	// the window or stick cursor back on last character.
	if (this->isScrolling)
		Window_ScrollUp(this, 0, this->height - 1, 1);
	else
		this->curx = this->width - 1;
	
	this->cury = this->height - 1; // Move back to the last line from one past the last line
}

void Window_ScrollUp(Window *win, int topline, int botline, int lines)
{
	TRACE("Window_ScrollUp");

	if (topline > botline) 
		return;

	for (int i = topline; i <= botline-lines; i++)
		memcpy(win->buffer[i], win->buffer[i+lines], sizeof(char_cell) * win->width);

	for (int i=0; i < win->width; i++)
		win->buffer[botline][i] = win->background;
}

void Window_ScrollDown(Window *win, int topline, int botline, int lines)
{
	TRACE("Window_ScrollDown");

	if (topline > botline) 
		return;

	for (int i=botline; i >= topline+lines; i--)
		memcpy(win->buffer[i], win->buffer[i-lines], sizeof(char_cell) * win->width);

	for (int i=0; i<win->width; i++)
		win->buffer[topline][i] = win->background;
}

PyObject *Window_scroll(Window *self, PyObject *args)
{
	TRACE("Window_scroll");

	int lines = 1;
	if(!PyArg_ParseTuple(args, "|i", &lines))
		return NULL;

	Window_ScrollUp(self, 0, self->height - 1, lines);
	Py_RETURN_NONE;
}

PyObject *Window_New(Window *parent, int begin_x, int begin_y, int width, int height, bool isPad)
{
	TRACE("Window_New");

	if ((width < 0) || (height < 0))
	{
		PyErr_SetString(CursesError, "Negative height or width given for new window.");
		return NULL;
	}

	if (parent != NULL)
	{
		if ((parent->isPad) != (isPad))
		{
			PyErr_SetString(PyExc_ValueError, "Parent and child options don't match.");
			return NULL;
		}

		// Test that the origin is in the parent
		if (!Window_ContainsPoint(parent, begin_x, begin_y))
		{
			PyErr_SetString(PyExc_ValueError, "Child's origin not in bounds of parent");
			return NULL;
		}

		printf("%d %d %d %d\n", begin_x, begin_y, width, height);

		// If default values are given for width or height, fill them in.
		width = width ? width : parent->width - begin_x;
		height = height ? height : parent->height - begin_y;

		// Test that the extent of the child is in the parent
		if (!Window_ContainsPoint(parent, begin_x + width -1, begin_y + height -1))
		{
			printf("%d %d %d %d\n", begin_x, begin_y, width, height);
			PyErr_SetString(PyExc_ValueError, "Child's extent not in bounds of parent");
			return NULL;
		}
	}

	Window *wo = PyObject_NEW(Window, &curses_windowType);
	if (!wo)
		return NULL;

	wo->background = SPACE;
	wo->attr = ATTR_BOLD;

	wo->backgd = wo->background | wo->attr;

	wo->isPad = isPad;

	// Set up our offset
	wo->offX=begin_x;
	wo->offY=begin_y;
	
	// Set window cursor to origin
	wo->curx=0;
	wo->cury=0;

	// Default option set
	wo->isScrolling=1;
	wo->noDelay=0;
	wo->_leave_cursor = 0;


	if (parent != NULL)
	{
		wo->parent = parent;
		wo->term = parent->term;

		wo->width = width;
		wo->height = height;

		// Allocate line pointers
		wo->buffer = (char_cell**)malloc(wo->height * sizeof(char_cell *));

		// Share buffer memory with parent
		for (int i=0; i < wo->height; i++)
		{
			wo->buffer[i] = &parent->buffer[wo->offY + i][wo->offX];
		}
	}
	else
	{
		wo->parent = NULL;
		wo->term = g_current_term;

		// Figure out this window's size
		wo->width=width ? width : wo->term->width;
		wo->height=height ? height : wo->term->height;

		// Allocate a buffer
		wo->buffer = AllocLines(wo->height, wo->width, (unsigned char)wo->background);
	}

	return (PyObject *)wo;
}

PyObject *Window_derwin(Window *self, PyObject *args)
{
	int begx = 0;
	int begy = 0;
	int width = 0; 
	int height = 0;

	TRACE("Window_derwin");

	switch(ARG_COUNT(args))
	{
	case 2:
		if (!PyArg_ParseTuple(args, "ii", &begy, begx))
			return NULL;

		break;

	case 4:
		if (!PyArg_ParseTuple(args, "iiii", &height, &width, &begy, begx))
			return NULL;

		break;

	default:
		PyErr_SetString(PyExc_TypeError, "derwin requires 2 or 4 arguments: [nlines, ncols], begin_y, begin_x");
		return NULL;
	}

	return Window_New(self, begx, begy, width, height, false);
}

PyObject *Window_subwin(Window *self, PyObject *args)
{
	int begx = 0;
	int begy = 0;
	int width = 0; 
	int height = 0;

	TRACE("Window_subwin");

	switch(ARG_COUNT(args))
	{
	case 2:
		if (!PyArg_ParseTuple(args, "ii", &begy, &begx))
			return NULL;
		break;

	case 4:
		if (!PyArg_ParseTuple(args, "iiii", &height, &width, &begy, &begx))
			return NULL;
		break;

	default:
		PyErr_SetString(PyExc_TypeError, "subwin requires 2 or 4 arguments: [nlines, ncols], begin_y, begin_x");
		return NULL;
	}

	// Change origin from screen to parent coordinates
	begx = begx - self->offX;
	begy = begy - self->offY;

	TRACE("Window_subwin: before Window_New");
	PyObject *wo = Window_New(self, begx, begy, width, height, false);
	TRACE("Window_subwin: after Window_New");

	return wo;
}

PyObject *Window_getbegyx(Window* self)
{
	TRACE("Window_getbegyx");
	return Py_BuildValue("(ii)", self->offY, self->offX);
}

PyObject *Window_getmaxyx(Window* self)
{
	TRACE("Window_getmaxyx");
	return Py_BuildValue("(ii)", self->height, self->width);
}

PyObject *Window_getyx(Window *self)
{
	TRACE("Window_getyx");
	return Py_BuildValue("(ii)", self->cury, self->curx);
}

// move the cursor position for this window
PyObject *Window_move(Window *self,PyObject *args)
{
	int newx,newy;
	if (!PyArg_ParseTuple(args,"ii", &newy,&newx))
    	return NULL;
    	
	Window_SetCur(self,newx,newy);
	Py_RETURN_NONE;
}

PyObject *get_keycode_from_buffer(PyObject *keybuffer)
{
	if (PyList_Size(keybuffer) == 0)
		return PyInt_FromLong((long) -1);

	// Take the next key from the front of the buffer
	PyObject *keycode = PyList_GetItem(keybuffer, 0);
	PySequence_DelItem(keybuffer, 0);
	return keycode;
}


PyObject *Window_getch(Window *self,PyObject * args)
{
	TRACE("Window_getch");
	
	int nargs = ARG_COUNT(args);
	if (nargs == 2)
	{
		int x, y;
		if (!PyArg_ParseTuple(args,"ii;y,x",&y,&x))
			return NULL;
			
		Window_SetCur(self,x,y);
	}
	else if (nargs != 0)
	{
		PyErr_SetString(PyExc_TypeError, "getch requires 0 or 2 arguments");
		return NULL;
	}

	// If there is a key in the pushback buffer, return that.
	if (self->term->ungetch != -1)
	{
		long ungot_character = self->term->ungetch;
		self->term->ungetch = -1;
		return PyInt_FromLong(ungot_character);
	}
	
	if (self->noDelay)
	{
		// Process all waiting messages, in case there's an input message in there.
		g_current_term->waiting_for_key = 1;
		ProcessMessages();
		g_current_term->waiting_for_key = 0;

		// If there still isn't a key in the buffer then bail.
		if(PyList_Size(g_current_term->keybuffer) == 0)
			return PyInt_FromLong((long) -1);
	}
	else
	{
		// wait for a key to pop into the buffer.
		g_current_term->waiting_for_key = 1;
		__int64 start, end, freq;

		QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
		QueryPerformanceCounter((LARGE_INTEGER*)&start);

		end = start;
		double diff = 0.0;
		
		while ((PyList_Size(g_current_term->keybuffer) == 0) && (!g_current_term->_halfdelay || (diff < 10)))
		{
			QueryPerformanceCounter((LARGE_INTEGER*)&end);
			diff = (double)(((end-start) * 10) / freq);

			WaitMessage();

			MSG msg;
			GetMessage(&msg,NULL,0,0);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		g_current_term->waiting_for_key = 0;
	}

	PyObject *keycode = get_keycode_from_buffer(g_current_term->keybuffer);

	if (self->term->_echo && (PyInt_AS_LONG(keycode) != -1))
	{
		PyObject *result;
		PyObject *args_none = Py_BuildValue("()");
		PyObject *args_one = Py_BuildValue("(O)", keycode);

		if (!self->isPad)
		{
			result = Window_refresh(self, args_none);
			CHECK(result);
		}

		result = Window_addch(self, args_one);
		CHECK(result);

		if (!self->isPad)
		{
			result = Window_refresh(self, args_none);
			CHECK(result);
		}

		Py_DECREF(args_none);
		Py_DECREF(args_one);
	}

	return keycode;
}

/*
	At the given coordinates, copy characters dist spaces to the right.
	Nothing is done to the first dist spaces.
*/
void Window_ScootLineRight(Window *win, int x, int y, int dist)
{
	for (int i = win->width-1; i >= x + dist; i--)
		win->buffer[y][i] = win->buffer[y][i - dist];
}

PyObject *Window_insch(Window *self, PyObject *args)
{
	int x = self->curx;
	int y = self->cury;
	int attr = self->attr;

	int ch;

	switch (ARG_COUNT(args))
	{
	case 1:
		if (!PyArg_ParseTuple(args, "i", &ch))
			return NULL;
	break;

	case 2:
		if (!PyArg_ParseTuple(args, "ii", &ch, &attr))
			return NULL;
	break;

	case 3:
		if (!PyArg_ParseTuple(args, "iii", &y, &x, &ch))
			return NULL;
	break;

	case 4:
		if (!PyArg_ParseTuple(args, "iiii", &y, &x, &ch, &attr))
			return NULL;
	break;

	default:
		PyErr_SetString(PyExc_TypeError, "insch requires 1-4 arguments");
		return NULL;
	}

	Window_SetCur(self, x, y);
	Window_ScootLineRight(self, x, y, 1);
	Window_AddChar(self, ch | attr, 0);
	Window_SetCur(self, x, y);

	Py_RETURN_NONE;
}

PyObject *Window_insstr(Window *self, PyObject *args)
{
	int attr;
	char *str;
	int str_len;

	int x = self->curx;
	int y = self->cury;

	switch (ARG_COUNT(args))
	{
	case 1:
		if (!PyArg_ParseTuple(args, "s#", &str, &str_len))
			return NULL;
	break;

	case 2:
		if (!PyArg_ParseTuple(args, "s#i", &str, &str_len, &attr))
			return NULL;
	break;

	case 3:
		if (!PyArg_ParseTuple(args, "iis#", &y, &x, &str, &str_len))
			return NULL;
	break;

	case 4:
		if (!PyArg_ParseTuple(args, "iis#i", &y, &x, &str, &str_len, &attr))
			return NULL;
	break;

	default:
		PyErr_SetString(PyExc_TypeError, "insstr requires 1-4 arguments");
		return NULL;
	}

	int max_index = min(str_len, self->width - x);
	Window_SetCur(self, x, y);
	
	for (int i = 0; i < max_index; i++)
	{

	}

	Window_SetCur(self, x, y);

	Py_RETURN_NONE;
}

PyObject *Window_addch(Window *self, PyObject *args)
{
	int x = 0;
	int y = 0;
	int attr = self->attr;
	PyObject *objch;

	switch (ARG_COUNT(args))
	{
		case 1:
			if (!PyArg_ParseTuple(args, "O", &objch))
				return NULL;
		break;

		case 2:
			if(!PyArg_ParseTuple(args, "Oi", &objch, &attr))
				return NULL;
			break;
		
		case 3:
			if (!PyArg_ParseTuple(args,"iiO", &y, &x, &objch))
				return NULL;

			self->curx = x;
			self->cury = y;
		break;

		case 4:
			if (!PyArg_ParseTuple(args,"iiOi", &y, &x, &objch, &attr))
				return NULL;

			self->curx = x;
			self->cury = y;
		break;

		default:
			PyErr_SetString(PyExc_TypeError, "addch takes 1-4 arguments: [y,x], ch, [attr]");
			return NULL;
	}

	char_cell ch;
	if (!ObjectToChar(objch, &ch))
	{
		PyErr_SetString(PyExc_TypeError, "Character should be an int or 1 char string");
		return NULL;
	}

	Window_AddChar(self,(attr | ch), 1);
	Py_RETURN_NONE;
}

PyObject *Window_addstr(Window *self, PyObject *args)
{
	char *str;
	int x,y;
	int attr = self->attr;
	
	switch (ARG_COUNT(args))
	{
	case 1:
		if(!PyArg_ParseTuple(args, "s;str", &str))
			return NULL;
		
		break;

	case 2:
		if(!PyArg_ParseTuple(args, "si;str", &str, &attr))
			return NULL;
		
		break;

	case 3:
		if(!PyArg_ParseTuple(args, "iis;str, y, x", &y, &x, &str))
			return NULL;

		self->curx = x;
		self->cury = y;

		break;

	case 4:
		if(!PyArg_ParseTuple(args, "iisi;str, y, x", &y, &x, &str, &attr))
			return NULL;

		self->curx = x;
		self->cury = y;

		break;

	default:
		PyErr_SetString(PyExc_TypeError, "addstr requires 1-4 arguments");
		return NULL;
	}

	while(*str)
	{
		Window_AddChar(self, (*str) | attr, 1);
		str++;
	}

	Py_RETURN_NONE;
}

PyObject *Window_noutrefresh(Window *self, PyObject *args)
{
	int pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol;
	int src_startx, src_starty, dest_startx, dest_starty, copy_width, copy_height;

	TRACE("Window_noutrefresh");

	if (self->isPad)
	{
		if(!PyArg_ParseTuple(args, "iiiiii", &pminrow, &pmincol, &sminrow, &smincol, &smaxrow, &smaxcol))
			return NULL;

		src_startx = pmincol;
		src_starty = pminrow;
		dest_startx = smincol;
		dest_starty = sminrow;
		copy_width = smaxcol - smincol + 1;
		copy_height = smaxrow - sminrow + 1;
	}
	else
	{
		if (!PyArg_ParseTuple(args, ""))
			return NULL;

		src_startx = src_starty = dest_startx = dest_starty = 0;
		copy_width = self->width;
		copy_height = self->height;
	}

	// Copy our line buffer to our terminal's screen buffer
	for (int y=0; y < copy_height; y++)
	{
		for (int x=0; x < copy_width; x++)
		{
			g_current_term->buffer[y+self->offY+dest_starty][x+self->offX+dest_startx]=self->buffer[y+src_starty][x+src_startx];
		}
	}

	if (!self->_leave_cursor)
		self->term->CursorPos(self->offX+self->curx, self->offY+self->cury);

	Py_RETURN_NONE;
}

PyObject *Window_refresh(Window *self, PyObject *args)
{
	PyObject *result;

	TRACE("Window_refresh");

	result = Window_noutrefresh(self, args);
	CHECK(result);

	result = Curses_doupdate(NULL);
	CHECK(result);

	Py_RETURN_NONE;
}

// move the window's location
PyObject *Window_mvwin(Window *self,PyObject * args)
{
	int newx, newy;
	if (!PyArg_ParseTuple(args,"ii;y,x", &newy, &newx))
		return NULL;

	self->offY=newy;
	self->offX=newx;

	Py_RETURN_NONE;    
}

PyObject *Window_insertln(Window *self, PyObject * args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	Window_ScrollDown(self, self->cury, self->height-1, 1);
	Py_RETURN_NONE;		
}

PyObject *Window_deleteln(Window *self, PyObject * args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	Window_ScrollUp(self, self->cury, self->height-1, 1);
	Py_RETURN_NONE;		
}


PyObject *Window_insdelln(Window *self,PyObject * args)
{
	int lines;
	if (!PyArg_ParseTuple(args, "i", &lines))
		return NULL;

	if (lines > 0)
		Window_ScrollDown(self, self->cury, self->height-1, lines);
	else if (lines < 0)
		Window_ScrollUp(self, self->cury, self->height-1, lines);

	Py_RETURN_NONE;		
}

PyObject *Window_clear(Window *self)
{
	TRACE("Window_clear");

	for (int y=0;y<self->height;y++)
		memset(self->buffer[y], self->background, self->width * sizeof(char_cell));

	Py_RETURN_NONE;		
}

PyObject *Window_clrtoeol(Window *self)
{
	TRACE("Window_clrtoeol");
	
	for (int x=self->curx;x<self->width;x++)
		self->buffer[self->cury][x]=self->background;

	Py_RETURN_NONE;		
}

PyObject *Window_clrtobot(Window *self)
{
	TRACE("Window_clrtobot");
	
	PyObject *result=Window_clrtoeol(self);
	CHECK(result);
		
	// clear all subsequent lines
	for (int y=self->cury+1; y < self->height; y++)
		memset(self->buffer[y], self->background,self->width);
	
	Py_RETURN_NONE;		

}

PyObject *Window_keypad(Window *self, PyObject *args)
{
	int keypad;
	if (!PyArg_ParseTuple(args, "i", &keypad))
		return NULL;

	self->term->_keypad = keypad;
	Py_RETURN_NONE;
}

PyObject *Window_leaveok(Window *self, PyObject *args)
{
	int leave_cursor;
	if(!PyArg_ParseTuple(args, "i", &leave_cursor))
		return NULL;

	self->_leave_cursor = leave_cursor;
	Py_RETURN_NONE;
}

PyObject *Window_nodelay(Window *self, PyObject *args)
{
	int nodelay;
	if(!PyArg_ParseTuple(args, "i", &nodelay))
		return NULL;

	self->noDelay = nodelay;
	Py_RETURN_NONE;
}

PyObject *Window_hline(Window *self, PyObject *args)
{
	int y, x, ch, n;

	TRACE("Window_hline");

	int attr = self->attr;

	switch(ARG_COUNT(args))
	{
	case 2:
		if(!PyArg_ParseTuple(args, "ii", &ch, &n))
			return NULL;
		break;

		x = self->curx;
		y = self->cury;
		
	case 4:
		if(!PyArg_ParseTuple(args, "iiii", &y, &x, &ch, &n))
			return NULL;
		break;

	default:
		PyErr_SetString(PyExc_TypeError, "hline requires 2 or 4 arguments");
		return NULL;
	}

	n = min(n, self->width - x);
	for (int i = x; i < x+n; i++)
	{
		self->buffer[y][i] = ch | attr;
	}

	Py_RETURN_NONE;
}

PyObject *Window_vline(Window *self, PyObject *args)
{
	TRACE("Window_vline");

	int y, x, ch, n;
	int attr = self->attr;

	switch(ARG_COUNT(args))
	{
	case 2:
		if(!PyArg_ParseTuple(args, "ii", &ch, &n))
			return NULL;
		break;

		x = self->curx;
		y = self->cury;
		
	case 4:
		if(!PyArg_ParseTuple(args, "iiii", &y, &x, &ch, &n))
			return NULL;
		break;

	default:
		PyErr_SetString(PyExc_TypeError, "vline requires 2 or 4 arguments");
		return NULL;
	}

	n = min(n, self->height - y);
	for (int i = y; i < y+n; i++)
		self->buffer[i][x] = ch | self->attr;

	Py_RETURN_NONE;
}

PyObject *Window_border_internal(Window *self, int ls, int rs, int ts, int bs, int tl, int tr, int bl, int br)
{
	PyObject *result;

	int savey = self->cury;
	int savex = self->curx;
	int savescrolling = self->isScrolling;

	ls = ls ? ls : ACS_VLINE;
	rs = rs ? rs : ACS_VLINE;
	ts = ts ? ts : ACS_HLINE;
	bs = bs ? bs : ACS_HLINE;
	tl = tl ? ts : ACS_ULCORNER;
	tr = tr ? tr : ACS_URCORNER;
	bl = bl ? bl : ACS_LLCORNER;
	br = br ? br : ACS_LRCORNER;

	self->isScrolling = 0;

	self->buffer[0][0] = tl | self->attr;
	self->buffer[0][self->width-1] = tr | self->attr;
	self->buffer[self->height-1][0] = bl | self->attr;
	self->buffer[self->height-1][self->width-1] = br | self->attr;
	
	result = Window_hline(self, Py_BuildValue("iiii", 0, 1, ts, self->width-2));
	CHECK(result);

	result = Window_hline(self, Py_BuildValue("iiii", self->height-1, 1, bs, self->width-2));
	CHECK(result);

	result = Window_vline(self, Py_BuildValue("iiii", 1, 0, ls, self->height-2));
	CHECK(result);

	result = Window_vline(self, Py_BuildValue("iiii", 1, self->width-1, rs, self->height-2));
	CHECK(result);

	self->cury = savey;
	self->curx = savex;
	self->isScrolling = savescrolling;

	Py_RETURN_NONE;
}

PyObject *Window_box(Window *self, PyObject *args)
{
	TRACE("Window_box");
	int vertch = 0, horch = 0;

	if(!PyArg_ParseTuple(args, "|ii", &vertch, &horch))
		return NULL;

	return Window_border_internal(self, vertch, vertch, horch, horch, 0,0,0,0);
}

PyObject *Window_border(Window *self, PyObject *args)
{
	TRACE("Window_border");
	int ls = 0;
	int rs = 0;
	int ts = 0;
	int bs = 0;
	int tl = 0;
	int tr = 0;
	int bl = 0;
	int br = 0;

	if(!PyArg_ParseTuple(args, "|iiiiii", &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br))
		return NULL;

	return Window_border_internal(self, ls, rs, ts, bs, tl, tr, bl, br);
}

PyObject *Window_attrset(Window *self, PyObject *args)
{
	TRACE("Window_attrset");

	int attr;
	if (!PyArg_ParseTuple(args, "i", &attr))
		return NULL;

	self->attr = attr;
	Py_RETURN_NONE;
}

PyObject *Window_attron(Window *self, PyObject *args)
{
	TRACE("Window_attron");

	int attr;
	if (!PyArg_ParseTuple(args, "i", &attr))
		return NULL;

	self->attr |= attr;
	Py_RETURN_NONE;
}

PyObject *Window_attroff(Window *self, PyObject *args)
{
	TRACE("Window_attroff");

	int attr;
	if (!PyArg_ParseTuple(args, "i", &attr))
		return NULL;

	self->attr &= ~attr;
	Py_RETURN_NONE;
}

PyObject  *Window_overlay(Window *self, PyObject *args)
{
	TRACE("Window_overlay");
	printf("overlay\n");

	Window *destwin;
	if(!PyArg_ParseTuple(args, "O!", &curses_windowType, &destwin))
		return NULL;

	int startx = max(self->offX, destwin->offX);
	int endx = min(self->offX + self->width -1, destwin->offX + destwin->width - 1);

	int starty = max(self->offY, destwin->offY);
	int endy = min(self->offY + self->height - 1, destwin->offY + destwin->height - 1);

	if ((endx < startx) || (endy < starty))
	{
		// No overlap
		Py_RETURN_NONE;
	}
	
	for (int iy = starty; iy <= endy; iy++)
	{
		for (int ix = startx; ix <= endx; ix++)
		{
			// Need to check for background character!

//			if ((destwin->buffer[iy - destwin->offY][ix - destwin->offX] & ATTR_CHAR) != destwin->background)
//			if ((destwin->buffer[iy - destwin->offY][ix - destwin->offX] & ATTR_CHAR) != ' ')
//			{
				destwin->buffer[iy - destwin->offY][ix - destwin->offX] =
						self->buffer[iy - self->offY][iy - self->offX];
//			}
//			else
//			{
//				destwin->buffer[iy - destwin->offY][ix - destwin->offX] = 'a';
//			}
		}
	}

	Py_RETURN_NONE;
}

void Window_Dealloc(Window *self)
{
	TRACE("Window_Dealloc");

	if (self->parent == NULL)
		FreeLines(self->buffer, self->height);

	PyObject_Del(self);
}

PyMethodDef Window_Methods[] = 
{
	{"addstr", (PyCFunction)Window_addstr, METH_VARARGS, "Draw the string to the window, possibly at (y,x), else at the cursor"},
	{"addch", (PyCFunction)Window_addch, METH_VARARGS, "Draw the character to window, possibly at (y,x), else at the cursor"},
	{"attroff", (PyCFunction)Window_attroff, METH_VARARGS, ""},
	{"attron", (PyCFunction)Window_attron, METH_VARARGS, ""},
	{"attrset", (PyCFunction)Window_attrset, METH_VARARGS, ""},
	{"border", (PyCFunction)Window_border, METH_VARARGS, "Draw a frame at the edge of the window"},
	{"box", (PyCFunction)Window_box, METH_VARARGS, "Draw a frame at the edge of the window"},
	{"clear", (PyCFunction)Window_clear, METH_NOARGS, "Clear the window"},
	{"clrtobot", (PyCFunction)Window_clrtobot, METH_NOARGS, "Clear from the cursor to the end of the window"},
	{"clrtoeol", (PyCFunction)Window_clrtoeol, METH_NOARGS, "Clear from the cursor to the end of the line"},
	{"deleteln", (PyCFunction)Window_deleteln, METH_VARARGS,"Delete the line under the cursor"},
	{"derwin", (PyCFunction)Window_derwin, METH_VARARGS, "Create a subwindow (origin given relative to parent)"},
	{"enclose", (PyCFunction)Window_enclose, METH_VARARGS, "Returns true if the window contains the given (y,x)"},
	{"erase", (PyCFunction)Window_clear, METH_NOARGS, "Clear the window"},
	{"getbegyx", (PyCFunction)Window_getbegyx, METH_NOARGS, "Get this window's origin (y,x)"},
	{"getch", (PyCFunction)Window_getch, METH_VARARGS, "Get one charater of input"},
	{"getmaxyx", (PyCFunction)Window_getmaxyx, METH_NOARGS, "Get this window's (height,width)"},
	{"getyx", (PyCFunction)Window_getyx, METH_NOARGS, "Get the cursor position (y,x) relative to this window's origin"},
	{"hline", (PyCFunction)Window_hline, METH_VARARGS, "Draw a horizontal line"},
	{"insch", (PyCFunction)Window_insch, METH_VARARGS, "Insert a character, moving the rest of the line to the right"},
	{"insdelln", (PyCFunction)Window_insdelln, METH_VARARGS, "For positive N, insert N lines; for negative N delete N lines"},
	{"insertln", (PyCFunction)Window_insertln, METH_VARARGS, "Insert a line at the cursor position"},
	{"leaveok", (PyCFunction)Window_leaveok, METH_VARARGS, "If called with true, updates to this window don't move hardware cursor"},
	{"keypad", (PyCFunction)Window_keypad, METH_VARARGS, "Determines if some keys with escape sequences are processed by curses (1) or left alone (0)"},
	{"move", (PyCFunction)Window_move, METH_VARARGS, "Move the cursor"},
	{"mvwin", (PyCFunction)Window_mvwin, METH_VARARGS, "Move the window's origin"},
	{"nodelay", (PyCFunction)Window_nodelay, METH_VARARGS, "If called with true, getch doesn't block; else getch blocks for input (default)"},
	{"noutrefresh", (PyCFunction)Window_noutrefresh, METH_VARARGS, ""},
	{"overlay", (PyCFunction)Window_overlay, METH_VARARGS, ""},
	{"refresh", (PyCFunction)Window_refresh, METH_VARARGS, "Refresh the window to the screen now"},
	{"scroll", (PyCFunction)Window_scroll, METH_VARARGS, "Scroll the window or scrolling region up N lines"},
	{"standend", (PyCFunction)Noop, METH_VARARGS, ""},
	{"standout", (PyCFunction)Noop, METH_VARARGS, ""},
	{"subwin", (PyCFunction)Window_subwin, METH_VARARGS, "Create a subwindow (origin given in screen coords)"},
	{"vline", (PyCFunction)Noop, METH_VARARGS, "Draw a horizontal line"},

	{"bkgdset", (PyCFunction)Noop, METH_VARARGS, ""},
	{"scrollok", (PyCFunction)Noop, METH_VARARGS, ""},
	/*
		Some functions are implmented as no-ops for now. 
		Because some curses functions don't make sense for windows 
		(timtout/notimeout), they will be left as no-ops in the final
		code release, though they will test for the appropriate number of
		arguments.

		Some functions are no-ops because we always redraw the whole screen.
	*/
	{"clearok", (PyCFunction)Noop, METH_VARARGS, ""},

	{"touchwin", (PyCFunction)Noop,	METH_VARARGS,""},
	{"touchline", (PyCFunction)Noop,	METH_VARARGS,""},
	
	{"timeout", (PyCFunction)Noop, METH_VARARGS, ""},
	{"notimeout", (PyCFunction)Noop, METH_VARARGS, ""},

	{"idcok", (PyCFunction)Noop, METH_VARARGS, "Sets if hardware character insert/delete is allowed. No-op."},
	{"idlok", (PyCFunction)Noop, METH_VARARGS, "Sets if hardware line insert/delete is allowed. No-op."},

	{"redrawln", (PyCFunction)Noop, METH_VARARGS, "Redraws lines between start and end on the next refresh."},
	{"redrawwin", (PyCFunction)Noop, METH_VARARGS, "Redraws the entire window on the next refresh."},

	{NULL}
};

PyTypeObject curses_windowType = 
{
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	"Window",	/*tp_name*/
	sizeof(Window),	/*tp_basicsize*/
	0,				/*tp_itemsize*/
	
	/* methods */
	(destructor)Window_Dealloc, 	/*tp_dealloc*/
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
	(getattrofunc)PyObject_GenericGetAttr,			/* tp_getattro */
	0,			/* tp_setattro */
	0,			/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,		/* tp_flags */
	0,		/* tp_doc */
	0,		/* tp_traverse */
	0,		/* tp_clear */
	0,		/* tp_richcompare */
	0,		/* tp_weaklistoffset */
	0,		/* tp_iter */
	0,		/* tp_iternext */

	Window_Methods, /* tp_methods */

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
