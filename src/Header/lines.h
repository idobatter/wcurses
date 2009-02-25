#ifndef _lines_h
#define _lines_h

#include "char_cell.h"

char_cell **AllocLines(int nlines, int ncols, char_cell default_char);
void FreeLines(char_cell **lines, int nlines);

#endif