#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>


extern Display *dpy;
extern Window root_window;

void* cursor_init();

void* cursor_move();