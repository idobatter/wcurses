# Copyright (c) 2003-2009, Flangy Software & Adam Vandenberg
# All rights reserved.
#
# See curses.license for license details.

import sys
from _WCurses import *

version = "WCurses 0.3 2008-02-25"
__version__ = version

# These should come from the size of the current terminal, not constants.
COLS = 80
LINES = 25

COLORS = 16
COLOR_PAIRS = 16

# Attribute bit flags
A_NORMAL = 0
A_BOLD = 0x00010000
A_STANDOUT = A_BOLD
A_UNDERLINE = 0
A_REVERSE = 0x40000000
A_BLINK = 0
A_DIM = 0
A_PROTECT = 0
A_INVIS = 0
A_ALTCHARSET = 0

# Attribute masks
A_CHARTEXT = 0x000000ff
A_COLOR = 0x0000ff00

# Colors
COLOR_BLACK = 0
COLOR_BLUE = 1
COLOR_GREEN = 2
COLOR_CYAN = 3
COLOR_RED = 4
COLOR_MAGENTA = 5
COLOR_YELLOW = 6
COLOR_WHITE = 7

_supported_keys = (
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_HOME,
    KEY_A1, KEY_A3,
    KEY_B2,
    KEY_C1, KEY_C3,

    KEY_IC, KEY_DC,
    KEY_F1, 
    KEY_F2, 
    KEY_F3, 
    KEY_F4, 
    KEY_F5, 
    KEY_F6, 
    KEY_F7, 
    KEY_F8, 
    KEY_F9, 
    KEY_F10, 
    KEY_F11, 
    KEY_F12,
    )

def has_key(key):
    return key in _supported_keys

def killchar():
	return None

def keyname(k):
	if 0 <= k <= 31:
		return '^'+chr(k+ord('A'))
	elif 32 <= k <= 127:
		return chr(k)
	elif 128 <= k <= 255:
		return 'M-' + keyname(k-128)
	else:
		raise ValueError()

def savetty():pass
def resetty():pass

def def_prog_mode(): pass
def def_shell_mode(): pass
def reset_prog_mode(): pass
def reset_shell_mode(): pass

def termattrs(): return 0

def filter(): pass

def qiflush(): pass
def noqiflush(): pass

def noraw(): pass
def raw(): pass

def isendwin(): return 0

# Wrapper stolen from curses.wrapper in the standard UNIX distribution
def wrapper(func, *rest):
	"""Wrapper function that initializes curses and calls another function,
	restoring normal keyboard/screen behavior on error.
	The callable object 'func' is then passed the main window 'stdscr'
	as its first argument, followed by any other arguments passed to
	wrapper().
	"""
	
	res = None
	try:
	# Initialize curses
		stdscr=initscr()
		print "init stdscr"
        
	# Turn off echoing of keys, and enter cbreak mode,
	# where no buffering is performed on keyboard input
		noecho()
		print "noecho"
        
		cbreak()
		print "cbreak"

	# In keypad mode, escape sequences for special keys
	# (like the cursor keys) will be interpreted and
	# a special value like curses.KEY_LEFT will be returned
		stdscr.keypad(1)
		print "keypad 1"

	# Start color, too.  Harmless if the terminal doesn't have
	# color; user can test with has_color() later on.  The try/catch
	# works around a minor bit of over-conscientiousness in the curses
	# module -- the error return from C start_color() is ignorable.
		try:
		    start_color()
		except:
		    pass
		    
		print "ready to go!"

		res = apply(func, (stdscr,) + rest)
	except:
	# In the event of an error, restore the terminal
	# to a sane state.
		stdscr.keypad(0)
		echo()
		nocbreak()
		endwin()
		
		# Pass the exception upwards
		(exc_type, exc_value, exc_traceback) = sys.exc_info()
		raise exc_type, exc_value, exc_traceback
	else:
	# Set everything back to normal
		stdscr.keypad(0)
		echo()
		nocbreak()
		endwin()		 # Terminate curses
		
		return res
