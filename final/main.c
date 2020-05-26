#include <wiringPi.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <X11/extensions/XTest.h>

#include "include/fbuff.h"

#define TIME
// #define SLOW

#define ITERATIONS 15
uint32_t iter = 0;
uint32_t diffs[ITERATIONS] = {0};
uint8_t* back_buffers[ITERATIONS] = {NULL};


void signal_callback() {
    printf("Change detected\n");
    if (diffs[iter] == 1) printf("Got two signals for some reason\n");
    diffs[iter] = 1;
}

int main() {
    Display *dpy;
    Window root_window;
    dpy = XOpenDisplay(0);
    if (dpy==NULL) {
        printf("DISPLAY environment variable not set");
        return -1;
    }
    root_window = XRootWindow(dpy, 0);
    XSelectInput(dpy, root_window, KeyReleaseMask);

    fbuff_dev_info_t* fbuff_dev = fbuff_init();
    long screensize = fbuff_dev->screensize;
    uint8_t* fbp = fbuff_dev->fbp;
    struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
    struct fb_fix_screeninfo finfo = fbuff_dev->finfo;

    uint8_t* original = malloc(screensize);
    memcpy(original, fbp, screensize);
    uint8_t* back_buffer = malloc(screensize);
    memcpy(back_buffer, original, screensize);

    int rows = fbuff_dev->rows;
    int cols = fbuff_dev->cols;

    // index with [x][y]
    uint32_t curr_brightness[rows][cols];
    memset((void*)curr_brightness, 0, rows*cols*4);
    uint32_t column = 0;
    uint32_t row = 0;
    uint64_t location;
    uint32_t color, red, blue, green, total;
    int32_t first_change[rows][cols];
    int32_t change[rows][cols];
    int32_t last_change[rows][cols];
     
#ifdef TIME 
    clock_t begin = clock(); 
#endif

    find_brightness(fbuff_dev, (uint32_t *)curr_brightness, back_buffer);

#ifdef TIME 
    clock_t end = clock();
    double time_spent_find_brightness = (double)(end - begin) / CLOCKS_PER_SEC;
    begin = clock();
#endif

    find_brightness_changes(fbuff_dev, (uint32_t*)curr_brightness, (uint32_t*)change);
    memcpy((void*)last_change, (void*)change, rows*cols*4);
    memcpy((void*)first_change, (void*)change, rows*cols*4);

#ifdef TIME
    end = clock();
    double time_spent_find_brightness_changes = (double)(end - begin) / CLOCKS_PER_SEC;
    begin = clock();
#endif

    update_buffer(fbuff_dev, (uint32_t*)change, back_buffer);

#ifdef TIME
    end = clock();
    double time_spent_update_buffer = (double)(end - begin) / CLOCKS_PER_SEC;
    begin = clock();
#endif 

    if (wiringPiSetup() == -1)
        return -1;
    pinMode(29, INPUT);
    pinMode(24, OUTPUT);

    wiringPiISR(29, INT_EDGE_RISING, (void*)&signal_callback);

    digitalWrite(24, HIGH);
    memcpy(fbp, back_buffer, screensize);
    digitalWrite(24, LOW);

#ifdef TIME
    end = clock();
    double time_spent_write_fb = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("%f, %f, %f, %f\n", time_spent_find_brightness, time_spent_find_brightness_changes, time_spent_update_buffer, time_spent_write_fb);
#endif
#ifdef SLOW
    sleep(1);
#endif
    fbuff_back_buffer_info_t* bbx = malloc(sizeof(fbuff_back_buffer_info_t));
    bbx->back_buffer = back_buffer;
    bbx->fbuff_dev = fbuff_dev;
    bbx->iteration = iter;
    bbx->change_vals = (int32_t*)(&first_change);

    for ( ; iter < ITERATIONS; ) {
        
        printf("next\n");
        // update_brightness_changes(fbuff_dev, iter, (int32_t*)change, (int32_t*)last_change );
        // update_buffer(fbuff_dev, (int32_t*)change, back_buffer);
        memcpy(back_buffer, original, screensize);
        fill_back_buffer(bbx);
        iter++;
        bbx->iteration++;
        digitalWrite(24, HIGH);
        memcpy(fbp, back_buffer, screensize);
        digitalWrite(24, LOW);
#ifdef SLOW
        sleep(1);
#endif
    }
    free((void*) bbx);
    sleep(1);
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    for (int j = 1; j < ITERATIONS; j+=2) {
        cursor_x += BOX_WIDTH * (cols/2 * diffs[j]) / ( 1 << (j/2));
        cursor_y += BOX_HEIGHT * (rows/2 * diffs[j+1]) / ( 1 << (j/2));
        printf("%d -> %d\n%d -> %d\n", diffs[j], cursor_x, diffs[j+1], cursor_y);
    }
    
    

    memcpy(fbp, original, screensize);
    XWarpPointer(dpy, None, root_window, 0, 0, 0, 0, cursor_x, cursor_y);
    // XFlush(dpy);
    XTestFakeButtonEvent(dpy, 1, True, CurrentTime);
    XTestFakeButtonEvent(dpy, 1, False, CurrentTime);
    XFlush(dpy);
    free(original);
    free(back_buffer);
    fbuff_deinit(fbuff_dev);
    return 0;
}
