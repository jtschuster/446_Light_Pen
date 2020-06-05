#include "../include/cursor.h"
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>
#include <X11/extensions/XTest.h>
#include <stdlib.h>
Display *dpy;
Window root_window;

void* cursor_init() 
{
    dpy = XOpenDisplay(0);
    if (dpy==NULL) {
        printf("DISPLAY environment variable not set");
        exit(-1);
        return NULL;
    }
    root_window = XRootWindow(dpy, 0);
    XSelectInput(dpy, root_window, KeyReleaseMask);
    return NULL;
}

void* cursor_move(int32_t cursor_x, int32_t cursor_y) 
{
    XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, cursor_x, cursor_y);
    // XFlush(dpy);
    XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
    XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
    XFlush(dpy);
    return NULL;
}