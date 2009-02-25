import NTCurses as curses
curses.initscr()
w2=curses.newwin(5,5,13,13)
w2.addstr("some text")
w2.addstr(" more text")
w2.addstr(" even more text")
w2.addstr(" even more text")
w2.addstr(" even more text")
w2.move(1,1)
w2.insertln()
w2.refresh()
w2.getch()
print "yo"
curses.endwin()
print "yo2"
