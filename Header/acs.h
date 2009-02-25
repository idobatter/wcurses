#ifndef _acs_h
#define _acs_h

#define ACS_BLOCK (0xdb)
// board of squares 
#define ACS_BOARD (0xb1)
// bullet 
#define ACS_BULLET (0xf9)
// checker board (stipple) 
#define ACS_CKBOARD (0xb1)
// arrow pointing down 
#define ACS_DARROW ('v')
// degree symbol
#define ACS_DEGREE (0xf8)
// diamond
#define ACS_DIAMOND ('*')
// greater-than-or-equal-to
#define ACS_GEQUAL (0xf2)
// lantern symbol 
#define ACS_LANTERN ('^')
// left arrow 
#define ACS_LARROW ('<')
// less-than-or-equal-to
#define ACS_LEQUAL (0xf3)
// not-equal sign
#define ACS_NEQUAL ('!') 
// letter pi
#define ACS_PI (0xe3)
// plus-or-minus sign 
#define ACS_PLMINUS (0xf1)
// right arrow 
#define ACS_RARROW ('>')
// scan line 1 
#define ACS_S1 ('-')
// scan line 3
#define ACS_S3 ('-') 
// scan line 7 
#define ACS_S7 ('-')
// scan line 9 
#define ACS_S9 ('_') 
// pound sterling 
#define ACS_STERLING (0x9c)
// up arrow
#define ACS_UARROW ('^')

// Box drawing characters
#define ACS_BTEE (0xc1)
#define ACS_HLINE (0xc4)
#define ACS_LLCORNER (0xc0)
#define ACS_LRCORNER (0xd9)
#define ACS_LTEE (0xc3)
#define ACS_PLUS (0xc5)
#define ACS_RTEE (0xb4)
#define ACS_TTEE (0xc2)
#define ACS_ULCORNER (0xda)
#define ACS_URCORNER (0xbf)
#define ACS_VLINE (0xb3)

// Alternate names for box drawing characters. Weird.
// Though I "get" the naming scheme.
#define ACS_BBSS (ACS_URCORNER)
#define ACS_BSBS (ACS_HLINE)
#define ACS_BSSB (ACS_ULCORNER)
#define ACS_BSSS (ACS_TTEE)
#define ACS_SBBS (ACS_LRCORNER)
#define ACS_SBSB (ACS_VLINE)
#define ACS_SBSS (ACS_RTEE)
#define ACS_SSBB (ACS_LLCORNER)
#define ACS_SSBS (ACS_BTEE)
#define ACS_SSSB (ACS_LTEE)
#define ACS_SSSS (ACS_PLUS)

#endif
