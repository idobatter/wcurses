#include <stdlib.h>
#include <string.h>
#include "lines.h"

char_cell **AllocLines(int nlines, int ncols, char_cell default_char)
{
	char_cell **lines;
	int i, j;

	lines = (char_cell**)malloc(nlines * sizeof(char_cell *));
	for (i=0; i < nlines; i++)
	{
		lines[i] = (char_cell*)malloc(ncols * sizeof(char_cell));
		for (j=0; j < ncols; j++)
		{
			lines[i][j] = default_char;
		}
	}

	return lines;
}

void FreeLines(char_cell **lines, int nlines)
{
	int i;

	for (i=0; i<nlines; i++)
	{
		free(lines[i]);
	}
	free(lines);
}
