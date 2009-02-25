#pragma once
#ifndef _curses_h
#define _curses_h

#include "Python.h"
#include "char_cell.h"

extern PyObject *CursesError;
PyObject *Curses_doupdate(PyObject *self);

extern PyMethodDef curses_methods[];

int ObjectToChar(PyObject *obj, char_cell *ch);
PyObject *NoArg_None(PyObject *self);
PyObject *NoArg_False(PyObject *self);
PyObject *NoArg_True(PyObject *self);

extern "C" 
{
	PyMODINIT_FUNC init_WCurses(void);
}


#endif
