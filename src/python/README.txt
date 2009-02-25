Curses for Windows Python extension.
4 May 2005

This binary was compiled with Microsoft Visual Studio .NET 2003, 
and linked against Python 2.3.5

This is a developer preview release of my Curses for Windows Python 
extension. This module should NOT be considered safe for production work.

Use of this code is done strictly AT YOUR OWN RISK.

This module does NOT yet implement the full API that the UNIX curses
extension does, and what it does implement is buggy (especially how it 
handles I/O options.

This package is distributed under the BSD license.


Latest downloads and information are available at:
http://adamv.com/dev/python/curses/


INSTALLATION:
Python 2.4.x for Windows comes with a lib\curses folder. Previous versions of Python 
omitted this folder in Windows installations. Its existence gets in the way of installing
this module.

Rename lib\curses to "curses-old", then copy the "curses" folder from this archive
to your site-packages folder.

I apologize for the lack of a distutils script.
