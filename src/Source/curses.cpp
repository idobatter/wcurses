/*
Copyright (c) 2003 Flangy Software & Adam Vandenberg
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer. 

* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution. 

* Neither the name of Flangy Software nor the names of its contributors 
may be used to endorse or promote products derived from this software 
without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <Mmsystem.h>
#include <commctrl.h>

#include "Python.h"
#include "pyhelper.h"

#include "Terminal.h"
#include "Window.h"
#include "curses_win32.h"

#include "acs.h"
#include "lines.h"
#include "char_cell.h"
#include "defines.h"

#include "globals.h"

#include "Trace.h"

PyObject *CursesError;

// Taken from _cursesmodule.c. This lets addch take string(1) or ints.
int ObjectToChar(PyObject *obj, char_cell *ch)
{
	if (PyInt_Check(obj))
	{
		*ch = (char_cell) PyInt_AsLong(obj);
		return 1;
	}

	if(PyString_Check(obj) && (PyString_Size(obj) == 1))
	{
		*ch = (char_cell) *PyString_AsString(obj);
		return 1;
	}
	
	return 0;
}

void crack_color(int color, int *r, int *g, int *b)
{
	*r = *g = *b = 0;

	if (color & RED_BIT) *r = 128;
	if (color & GREEN_BIT) *g = 128;
	if (color & BLUE_BIT) *b = 128;
}


void InitializeColors()
{
	g_color_pairs[0] = 7;

	for (int i=1; i < 256; i++)
	{
		g_color_pairs[i] = 0;
	}
}

PyObject *Curses_longname(PyObject *self)
{
	return PyString_FromString(TerminalLongName);
}

PyObject *Curses_ConsoleSize(PyObject *self, PyObject *args)
{
	int nlines, ncols;

	TRACE("Curses_ConsoleSize");
	
	if (g_called_initscr)
		return NULL;
	
	if (!PyArg_ParseTuple(args,"ii;nlines,ncols",	&nlines,&ncols))
    	return NULL;

	Py_RETURN_NONE;
}


PyObject *Curses_newwin(PyObject *self, PyObject *args)
{
	int nlines = 0;
	int ncols = 0;
	int begin_y = 0;
	int begin_x = 0;

	TRACE("Curses_newwin");

	switch (ARG_COUNT(args)) 
	{
		case 2:
			if (!PyArg_ParseTuple(args,"ii;begin_y,begin_x", &begin_y,&begin_x))
				return NULL;
			break;

		case 4:
			if (!PyArg_ParseTuple(
				args,"iiii;nlines,ncols,begin_y,begin_x", &nlines,&ncols,&begin_y,&begin_x))
			{
				return NULL;
			}
			break;

		default:
				PyErr_SetString(PyExc_TypeError, "newwin requires 2 or 4 arguments");
				return NULL;
	}
	
	return (PyObject *)Window_New(NULL, begin_x, begin_y, ncols, nlines, false);
}


PyObject *Curses_newpad(PyObject *self, PyObject *args)
{
	int nlines = 0;
	int ncols = 0;

	TRACE("Curses_newpad");

	if (!PyArg_ParseTuple(args, "ii", &nlines, &ncols))
		return NULL;

	return (PyObject *)Window_New(NULL, 0, 0, ncols, nlines, true);
}


PyObject *Curses_endwin(PyObject *self,PyObject *args)
{
	TRACE("Curses_endwin");

	if (!g_called_initscr)
	{
		PyErr_SetString(CursesError, "endwin called before initscr or newterm");
		return NULL;
	}

	if (g_current_term != NULL)
		Py_DECREF(g_current_term);

	Py_RETURN_NONE;
}

PyObject *Curses_doupdate(PyObject *self)
{
	TRACE("Curses_doupdate");
	g_current_term->RedrawText();
	Py_RETURN_NONE;
}

PyObject *Curses_init_pair(PyObject *self, PyObject *args)
{
	int pair;
	int fg, bg;
	int color;

	TRACE("Curses_init_pair");

	if(!PyArg_ParseTuple(args, "iii", &pair, &fg, &bg))
		return NULL;

	color = (bg << 4) | fg;
	g_color_pairs[pair] = color;

//	printf("%d: %d = %d/%d\n", pair, color, fg, bg);

	Py_RETURN_NONE;
}

PyObject *Curses_color_pair(PyObject *self, PyObject *args)
{
	int which_pair;
	if(!PyArg_ParseTuple(args, "i", &which_pair))
		return NULL;

	return PyInt_FromLong(which_pair << 8);
}

PyObject *Curses_curs_set(PyObject *self, PyObject *args)
{
	int visibility;

	TRACE("Curses_cur_set");

	if (!PyArg_ParseTuple(args, "i", &visibility))
		return NULL;

	if ((visibility < 0) || (2 < visibility))
	{
		PyErr_SetString(PyExc_ValueError, "visibility should be 0, 1, or 2");
		return NULL;
	}

	return PyInt_FromLong(g_current_term->SetCursorVisibility(visibility));
}


PyObject *Curses_napms(PyObject *self, PyObject *args)
{
	int nap_time_ms;

/*
	Actually tracing this will spew a LOT of napms messages to the terminal.
	TRACE("Curses_napms");
*/

	if(!PyArg_ParseTuple(args, "i", &nap_time_ms))
		return NULL;

	if ((nap_time_ms !=0) && (nap_time_ms != INFINITE))
		Sleep(nap_time_ms);

	Py_RETURN_NONE;
}

PyObject *Curses_flash(PyObject *self)
{
	TRACE("Curses_flash");
	FlashWindow(g_current_term->win, TRUE);
	Py_RETURN_NONE;
}

PyObject *Curses_beep(PyObject *self)
{
	TRACE("Curses_beep");
	MessageBeep(MB_OK);
	Py_RETURN_NONE;
}

PyObject *Curses_echo(PyObject *self)
{
	g_current_term->_echo = 1;
	Py_RETURN_NONE;
}

PyObject *Curses_noecho(PyObject *self)
{
	g_current_term->_echo = 0;
	Py_RETURN_NONE;
}

PyObject *Curses_cbreak(PyObject *self)
{
	g_current_term->_cbreak = 1;
	Py_RETURN_NONE;
}

PyObject *Curses_nocbreak(PyObject *self)
{
	g_current_term->_cbreak = 0;
	g_current_term->_halfdelay = 0;
	Py_RETURN_NONE;
}

PyObject *Curses_halfdelay(PyObject *self, PyObject *args)
{
	int delay;
	if (!PyArg_ParseTuple(args, "i", &delay))
		return NULL;

	g_current_term->_halfdelay = delay;
	Py_RETURN_NONE;
}

PyObject *Curses_erasechar(PyObject *self)
{
	return PyInt_FromLong(BACKSPACE);
}

PyObject *Curses_meta(PyObject *self, PyObject *args)
{
	int meta;
	if(!PyArg_ParseTuple(args, "i", &meta))
		return NULL;

	g_current_term->allow8bitInput = meta;
	Py_RETURN_NONE;
}

PyObject *Curses_baudrate(PyObject *self)
{
	return PyInt_FromLong(200000);
}

PyObject *Curses_initscr(PyObject *self)
{
	TRACE("Curses_initscr");

	if (g_called_initscr)
	{
		PyErr_SetString(CursesError, "Curses already initialized!");
		return NULL;
	}

	g_called_initscr = 1;

	g_default_term = Terminal::New();
	// Keep an extra reference to the default terminal, because we try to decref it when the DLL unloads.
	Py_INCREF(g_default_term);
	//Terminal_Show(g_default_term);
	g_default_term->Show();

	// Set the current terminal to the default one.
	g_current_term = g_default_term;

    return Window_New(NULL, 0,0,0,0, false);
}

PyObject *Curses_flushinp(PyObject *self)
{
	PySequence_DelSlice(g_current_term->keybuffer, 0, -1);
	g_current_term->ungetch = -1;
	Py_RETURN_NONE;
}

PyObject *Curses_ungetch(PyObject *self, PyObject *args)
{
	int ch;
	if(!PyArg_ParseTuple(args, "i", &ch))
		return NULL;

	if (g_current_term->ungetch != -1)
	{
		PyErr_SetString(CursesError, "There is already a character in the pushback buffer.");
		return NULL;
	}

	g_current_term->ungetch = ch;
	Py_RETURN_NONE;
}


PyObject *Noop(PyObject *self,PyObject *args)
{
	Py_RETURN_NONE;
}

PyObject *NoArg_None(PyObject *self)
{
	Py_RETURN_NONE;
}

PyObject *NoArg_False(PyObject *self)
{
	Py_RETURN_FALSE;
}

PyObject *NoArg_True(PyObject *self)
{
	Py_RETURN_TRUE;
}


static char curs_set_doc[] =
	"Sets the cursor visibility level.\n"
	"0: Invisible\n"
	"1: Visible (underline)\n"
	"2: Very visible (block, default)\n";

PyMethodDef curses_methods[] = 
{
	{"_ConsoleSize", (PyCFunction)Curses_ConsoleSize, METH_VARARGS, "Call prior to initscr to set the console size"},

	{"baudrate", (PyCFunction)Curses_baudrate, METH_NOARGS, "Returns a big number (deprecated)"},
	{"beep", (PyCFunction)Curses_beep, METH_NOARGS, "Beep"},
	{"can_change_color", (PyCFunction)NoArg_False, METH_NOARGS, "Returns true if you can change terminal colors (not yet)"},
	{"cbreak", (PyCFunction)Curses_cbreak, METH_NOARGS, "Puts the terminal into cbreak mode"},
	{"color_pair", (PyCFunction)Curses_color_pair, METH_VARARGS, "Return the attr value for the given color pair"},
	{"curs_set", (PyCFunction)Curses_curs_set, METH_VARARGS, curs_set_doc},
	{"delay_output", (PyCFunction)Curses_napms, METH_VARARGS, "Adds a delay to the output (uh, calls napms)"},
	{"doupdate", (PyCFunction)Curses_doupdate, METH_NOARGS, "Copy the terminal's buffer to the screen"},
	{"echo", (PyCFunction)Curses_echo, METH_NOARGS, "Characters are echoed to the screen as they are typed"},
	{"endwin", (PyCFunction)Curses_endwin, METH_NOARGS, "End the current terminal"},
	{"erasechar", (PyCFunction)Curses_erasechar, METH_NOARGS, "Returns the code for backspace (which is 8)"},
	{"flash", (PyCFunction)Curses_flash, METH_NOARGS, "Flash the screen (via flashing the Window's titlebar)"},
	{"flushinp", (PyCFunction)Curses_flushinp, METH_NOARGS, "Flush any buffered input"},
	{"has_colors", (PyCFunction)NoArg_True, METH_NOARGS, "Returns true if this library supports colors (not yet)"},
	{"has_ic", (PyCFunction)NoArg_True, METH_NOARGS, "Returns true if the terminal has insert- and delete- character capabilities. (Always True)"},
	{"has_il", (PyCFunction)NoArg_True, METH_NOARGS, "Returns true if the terminal has insert- and delete-line capabilities. (Always True)"},
	{"halfdelay", (PyCFunction)Curses_halfdelay, METH_VARARGS, "Puts the terminal into half delay mode"},
	{"init_pair", (PyCFunction)Curses_init_pair, METH_VARARGS, "Change the colors in the given slot"},
	{"initscr", (PyCFunction)Curses_initscr, METH_NOARGS, "Creates a new default terminal and a full-screen window for it"},
	{"longname", (PyCFunction)Curses_longname, METH_NOARGS, "Returns a long description of the terminal"},
	{"meta", (PyCFunction)Curses_meta, METH_VARARGS, "If passed true, allow 8-bit character input (default), if false disallow"},
	{"napms", (PyCFunction)Curses_napms, METH_VARARGS, "Sleep for ms milliseconds"},
	{"newpad", (PyCFunction)Curses_newpad, METH_VARARGS, "Create a new pad on the current terminal"},
	{"newwin", (PyCFunction)Curses_newwin, METH_VARARGS, "Create a new window on the current terminal"},
	{"nl", (PyCFunction)NoArg_None, METH_NOARGS, ""},
	{"nocbreak", (PyCFunction)Curses_nocbreak, METH_NOARGS, "Takes the terminal out of cbreak mode"},
	{"noecho", (PyCFunction)Curses_noecho, METH_NOARGS, "Characters are not echoed to the screen as they are typed"},
	{"nonl", (PyCFunction)NoArg_None, METH_NOARGS, ""},
	{"start_color", (PyCFunction)NoArg_None, METH_NOARGS, "Must be called after initscr() to use colors"},
	{"ungetch", (PyCFunction)Curses_ungetch, METH_VARARGS, "Push back one character of input. Throws an exception if there is already a character in the pushback buffer."},

	{NULL}
};
