import curses

def bottom_panel():
    global _panel_stack

    try:
        return _panel_stack[0]
    except:
        return None
    
def top_panel():
    global _panel_stack

    try:
        return _panel_stack[-1]
    except:
        return None
    
def new_panel(win):
    return Panel(win)

def update_panels():
    global _panel_stack

    for panel in _panel_stack:
        win = panel.window()
        if not win.hidden():
            win.noutrefresh()


class Panel:
    def __init__(self, win):
        self.win = win
        self.userdata = None
        self.hidden = 0

    def __del__(self):
#        global _panel_stack
#        _panel_stack.remove(self)
        pass
        

    def window(self):
        return self.win

    def set_userptr(self, userdata):
        self.userdata = userdata

    def userptr(self):
        return self.userdata

    def hidden(self):
        return self.hidden

    def hide(self):
        self.hidden = 1

    def show(self):
        self.hidden = 0

    def move(self, y, x):
        sef.win.move(y,x)

    def replace(self, win):
        self.win = win

    def above(self):
        global _panel_stack

        index = _panel_stack.index(self)
        if index == len(_panel_stack) - 1:
            return None
        else:
            return _panel_stack[index+1]

    def below(self):
        global _panel_stack

        index = _panel_stack.index(self)
        if index == 0:
            return None
        else:
            return _panel_stack[index-1]

    def bottom(self):
        global _panel_stack

        _panel_stack.remove(self)
        _panel_stack.insert(0,self)

    def top(self):
        global _panel_stack

        _panel_stack.remove(self)
        _panel_stack.append(self)


# Please don't access this directly you knob.
# [0] = bottom, [-1] = top
_panel_stack = list()
