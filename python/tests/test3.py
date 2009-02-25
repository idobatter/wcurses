import curses
print "Imported"

w1 = curses.initscr()
print "initscr"

curses.cbreak()
curses.halfdelay(10)
while True:
	q = w1.getch()
	print q
	if q == ord('q'): break

curses.endwin()
print "endwin"

print "Done."
