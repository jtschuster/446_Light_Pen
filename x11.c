#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    Display *display;
    Window window;
    XEvent event;
    XWindowAttributes attributes;
    Colormap colormap;
    

    char *msg = "Hello, World!";
    int s;
    Screen *screen;
    // open connection to the server
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }
    s = DefaultScreen(display);
    screen = XScreenOfDisplay(display, s);

    int SCREEN_WIDTH = XWidthOfScreen(screen);
    int SCREEN_HEIGHT =XHeightOfScreen(screen);
    
    // create window
    window = XCreateSimpleWindow(display, RootWindow(display, s), 10, 10, XWidthOfScreen(screen), XHeightOfScreen(screen), 10,
                                 BlackPixel(display, s), WhitePixel(display, s));
    
    // select kind of events we are interested in
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    
    // Fullscreen stuff
    XSizeHints* size_hints;
    long hints = 0;

    size_hints = XAllocSizeHints();

    if (XGetWMSizeHints(display, window, size_hints, &hints,
        XInternAtom(display, "WM_SIZE_HINTS", False)) == 0) {
        puts("Failed.");
    } 

    XLowerWindow(display, window);
    XUnmapWindow(display, window);
    XSync(display, False);

    printf("%ld\n", hints);

    XFree(size_hints);

    Atom atoms[2] = { XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False), None };

    XChangeProperty(
        display,
        window,
        XInternAtom(display, "_NET_WM_STATE", False),
        XA_ATOM, 32, PropModeReplace, (unsigned char*)atoms, 1);
    // end fullscreen stuff

    // map (show) the window

    XGCValues *gc_vals;
    XMapWindow(display, window);
    GC gc = DefaultGC(display, s);
    GC gcs[8];

    gcs[0] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[0], 0x00000000L);

    gcs[1] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[1], 0x000000FFL);

    gcs[2] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[2], 0x0000FF00L);

    gcs[3] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[3], 0x0000FFFFL);

    gcs[4] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[4], 0x00FF0000L);

    gcs[5] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[5], 0x00FF00FFL);

    gcs[6] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[6], 0x00FFFF00L);

    gcs[7] = XCreateGC(display, window, 0, gc_vals);
    XSetForeground(display, gcs[7], 0x00FFFFFFL);
    
    XSetForeground(display, gc, 0x00FFFF00L);
    // event loop

    int placement[6] = {1, 2, 3, 4, 5, 6};
    int height, width, x, y, pwidth, pheight, tall;
    height = SCREEN_HEIGHT;
    width = SCREEN_WIDTH;
    int xoffset = 0;
    int yoffset = 0; 
    for (int k = 0; k <= 6;)
    {
        printf("%d\n", k);
        XNextEvent(display, &event);
        // draw or redraw the window
        if (width > height) {
            width = width / 4;
            height = height / 2;
            tall = 0;
        }else {
            width = width / 2;
            height = height / 4;
            tall=1;
        }
        for(int j = 0; j < 8; j++) {
            if (tall) {
                x = width * (j & 1);
                y = height * (j >> 1);
            } else {
                x = width * (j >> 1);
                y = height * (j & 1);
            }
            // width = SCREEN_WIDTH / (4 << (2*k));
            // height = SCREEN_HEIGHT / (2 << k);
            // x = width * (j >> 1);
            // y = height * (j & 1);
            // if (width / height > 1 || height / width > 1) {
            //     width = width * 2;
            //     height = height / 2;
            //     x = width * (j & 1);
            //     y = height * (j >> 1);
            //     printf("asdfasdf\n");
            // }
            XFillRectangle(display, 
                            window, 
                            gcs[j],
                            x + xoffset, 
                            y + yoffset, 
                            width,
                            height);
            
        } 
        xoffset += width  * (tall ? placement[k] / 4 : placement[k] % 4);
        yoffset += height * (tall ? placement[k] % 4 : placement[k] / 4);

        // exit on key press
        k++;
    }
 
    // close connection to the server
    XCloseDisplay(display);
 
    return 0;
 }
