#include "Python.h"
struct Terminal;

PyObject *g_keymap_dict;
int g_called_initscr;

Terminal *g_default_term;
Terminal *g_current_term;

int g_color_pairs[256];

char *key_names[];
