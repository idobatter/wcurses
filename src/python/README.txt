Curses for Windows Python extension.


This binary was compiled with Microsoft Visual Studio 2008, 
and linked against Python 2.5.2.

This is a developer preview release of my Curses for Windows Python 
extension. This module should NOT be considered safe for production work.

Use of this code is done strictly AT YOUR OWN RISK.

This module does NOT yet implement the full API that the UNIX curses
extension does, and what it does implement is buggy (especially how it 
handles I/O options.

This package is distributed under the BSD license.


Latest downloads and information are available at:
http://code.google.com/p/wcurses/


INSTALLATION:
Python 2.5.x for Windows comes with a lib\curses folder. 
(Previous versions of Python omitted this folder in Windows installations.)

Rename lib\curses to "curses-old", then copy the "curses" folder from this 
archive to your site-packages folder. You may try copying some of the support 
modules from old back.
