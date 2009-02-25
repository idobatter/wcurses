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

#include "curses.h"
#include "terminal.h"
#include "window.h"

#include "acs.h"
#include "defines.h"

#include "globals.h"

#include "Trace.h"

struct keymap_t
{
	int win_key;
	char *curses_name;
	int curses_key;
};

struct keymap_t g_win_keymap[] = 
{
	{VK_UP, "KEY_UP", 259},
	{VK_DOWN, "KEY_DOWN", 258},
	{VK_LEFT, "KEY_LEFT", 260},
	{VK_RIGHT, "KEY_RIGHT", 261},
	{VK_HOME, "KEY_HOME", 262},

	{VK_INSERT, "KEY_IC", 331},
	{VK_DELETE, "KEY_DC", 330},

	{VK_HOME, "KEY_A1", 262},
	{VK_PRIOR, "KEY_A3", 339},
	{VK_CLEAR, "KEY_B2", 350},
	{VK_END, "KEY_C1", 360},
	{VK_NEXT, "KEY_C3", 338},

	{VK_F1, "KEY_F1", 265},
	{VK_F2, "KEY_F2", 266},
	{VK_F3, "KEY_F3", 267},
	{VK_F4, "KEY_F4", 268},
	{VK_F5, "KEY_F5", 269},
	{VK_F6, "KEY_F6", 270},
	{VK_F7, "KEY_F7", 271},
	{VK_F8, "KEY_F8", 272},
	{VK_F9, "KEY_F9", 273},
	{VK_F10, "KEY_F10", 274},
	{VK_F11, "KEY_F11", 275},
	{VK_F12, "KEY_F12", 276},

	{0,NULL,0}
};


/*
	Add KEY_* defines to the module dict.
	We add all strings in key_names to the namespace, 
	giving them unique integer values.
*/
void InitKeynames(PyObject *dict)
{
	TRACE("InitKeynames");

	// Add known keys
	for (int i=0; g_win_keymap[i].win_key != 0; i++)
	{
		PyDict_SetItemString(dict, g_win_keymap[i].curses_name, PyInt_FromLong(g_win_keymap[i].curses_key));
	}

	// Add unknown keys
#define KEYSTART 1024
	for (int i=0; key_names[i] != NULL; i++)
	{
		if (PyDict_GetItemString(dict, key_names[i]) == NULL)
			PyDict_SetItemString(dict, key_names[i], PyInt_FromLong(i + KEYSTART));
	}
#undef KEYSTART
}

/*
	Set up a map of Windows VK_ values to Curses KEY_ values.
	WM_KEYDOWN processing looks up the given VK_ in the map;
	if found it adds the KEY_ value to the keybuffer, 
	otherwise the key is ignored.

	Additional KEY_ mapping may take place in WM_CHAR
*/
void InitKeymap(PyObject *dict)
{
	PyObject *curseskeyvalue;
	int i;

	TRACE("InitKeymap");

	g_keymap_dict = PyDict_New();

	for (i=0; g_win_keymap[i].win_key != 0; i++)
	{
		curseskeyvalue = PyDict_GetItemString(dict, g_win_keymap[i].curses_name);
		PyDict_SetItem(g_keymap_dict, PyInt_FromLong(g_win_keymap[i].win_key), curseskeyvalue);
	}
}

PyMODINIT_FUNC init_WCurses(void)
{
	PyObject *module, *mdict;

	if (PyType_Ready(&curses_terminalType) < 0) return;
	if (PyType_Ready(&curses_windowType) < 0) return;

	module = Py_InitModule("_WCurses", curses_methods);
	mdict = PyModule_GetDict(module);

	InitKeynames(mdict);
	InitKeymap(mdict);

	CursesError = PyErr_NewException("curses.error", NULL, NULL);
	PyDict_SetItemString(mdict, "error", CursesError);

#define AddConstant(value) \
	PyDict_SetItemString(mdict, #value, PyInt_FromLong( (value) ));

#define AddConstantNV(name, value) \
	PyDict_SetItemString(mdict, name, PyInt_FromLong( (value) ));

//	AddConstantNV("ERR", CURSES_ERR);
//	AddConstantNV("OK",CURSES_OK);

	AddConstant(ACS_BLOCK);
	AddConstant(ACS_BOARD);
	AddConstant(ACS_BULLET);
	AddConstant(ACS_CKBOARD);
	AddConstant(ACS_DARROW);
	AddConstant(ACS_DEGREE);
	AddConstant(ACS_DIAMOND);
	AddConstant(ACS_GEQUAL);
	AddConstant(ACS_LANTERN);
	AddConstant(ACS_LARROW);
	AddConstant(ACS_LEQUAL);
	AddConstant(ACS_NEQUAL);
	AddConstant(ACS_PI);
	AddConstant(ACS_PLMINUS);
	AddConstant(ACS_RARROW);
	AddConstant(ACS_S1);
	AddConstant(ACS_S3);
	AddConstant(ACS_S7);
	AddConstant(ACS_S9);
	AddConstant(ACS_STERLING);
	AddConstant(ACS_UARROW);

	AddConstant(ACS_BTEE);
	AddConstant(ACS_HLINE);
	AddConstant(ACS_LLCORNER);
	AddConstant(ACS_LRCORNER);
	AddConstant(ACS_LTEE);
	AddConstant(ACS_PLUS);
	AddConstant(ACS_RTEE);
	AddConstant(ACS_TTEE);
	AddConstant(ACS_ULCORNER);
	AddConstant(ACS_URCORNER);
	AddConstant(ACS_VLINE);

	AddConstant(ACS_BBSS);
	AddConstant(ACS_BSBS);
	AddConstant(ACS_BSSB);
	AddConstant(ACS_BSSS);
	AddConstant(ACS_SBBS);
	AddConstant(ACS_SBSB);
	AddConstant(ACS_SBSS);
	AddConstant(ACS_SSBB);
	AddConstant(ACS_SSBS);
	AddConstant(ACS_SSSB);
	AddConstant(ACS_SSSS);

#undef AddConstant
#undef AddConstantNV
}
