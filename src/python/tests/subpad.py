import curses

stdscr = curses.initscr()
pad = curses.newpad(100,100)
for i in range(0,99):
	for j in range (0,99):
		pad.addch(j,i,chr(((i+j)%26)+ord('A')))

pad.refresh(0,0,1,1,3,3)
pad.refresh(1,1,1,5,3,7)
stdscr.getch()

