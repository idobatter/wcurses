#pragma once

extern HINSTANCE g_dll_instance;
extern WNDCLASSEX  g_terminal_wndclass;

extern LOGFONT g_default_font;

extern const char *TerminalLongName;
extern const char *TerminalProp;
extern const char *Version;

void RegisterTerminalWindowClass();
void ProcessMessages();

void crack_color(int color, int *r, int *g, int *b);
void InitializeColors();
