#include <wiringPi.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/X.h>

#include <pthread.h>
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
#include <signal.h>
#include <sys/time.h>

#include "include/fbuff.h"
#include "include/cursor.h"

#define TIME
// #define SLOW
#define FRAME_DELAY 33333 //microseconds

#define ITERATIONS 16 // gotta be odd number
uint32_t iter = 0;
uint32_t diffs[ITERATIONS] = {0};
fbuff_dev_info_t* fbuff_dev;
volatile int32_t running = 1;
void signal_callback()
{
    printf("Change detected\n");
    if (iter < ITERATIONS){
        if (diffs[iter] == 1) printf("Got two signals for some reason\n");
        diffs[iter] = 1;
    } else {
        printf("Got a signal after the movement that woulda overshot an array\n");
    }
}

void sigint_callback(int unused) {
    running = 0;
}

int run_light_pen()
{
    const long screensize = fbuff_dev->screensize;
    uint8_t* fbp = fbuff_dev->fbp;
    struct timeval start, stop;
    int rows = fbuff_dev->rows;
    int cols = fbuff_dev->cols;
    uint32_t delay = 0.0;
    signal(SIGINT, sigint_callback);
    gettimeofday(&start, NULL);
    for (iter = 0 ; iter < ITERATIONS; iter++) {
        printf("next\n");
        digitalWrite(24, HIGH);
        memcpy(fbp, *(fbuff_dev->bb_array + iter), screensize);
        digitalWrite(24, LOW);
        do {
            gettimeofday(&stop, NULL);
            delay = (stop.tv_usec - start.tv_usec); 
        } while (delay < FRAME_DELAY);
        gettimeofday(&start, NULL);
        printf("Did we get stalled by the thread? %f\n", delay / 1000000.0);
#ifdef SLOW
        sleep(1);
#endif
    }
    while (delay < FRAME_DELAY){
        gettimeofday(&stop, NULL);
        delay = (stop.tv_usec - start.tv_usec); 
    }
    int32_t cursor_x = 0;
    int32_t cursor_y = 0;
    for (int j = 0; j < ITERATIONS; j+=2) {
        cursor_x += BOX_WIDTH * (cols/2 * diffs[j]) / ( 1 << (j/2));
        cursor_y += BOX_HEIGHT * (rows/2 * diffs[j+1]) / ( 1 << (j/2));
        printf("%d -> %d\n%d -> %d\n", diffs[j], cursor_x, diffs[j+1], cursor_y);
    }
    
    memcpy(fbp, fbuff_dev->original, screensize);
    cursor_move(cursor_x, cursor_y);
    memset(diffs, 0, ITERATIONS*4);
    return 0;
}

void button_callback() {
    run_light_pen();
}

int main() {

    cursor_init();

    fbuff_dev = fbuff_init(ITERATIONS);

    if (wiringPiSetup() == -1)
        return -1;
    pinMode(29, INPUT); //* Signal from ADC. High when change threshold reached
    pinMode(24, OUTPUT); //* Signal for debugging showing when an iteration begins
    pinMode(11, INPUT); //* Signal from button to start light pen running
    pullUpDnControl (11, PUD_UP);
    wiringPiISR(29, INT_EDGE_RISING, (void*)&signal_callback);
    wiringPiISR(11, INT_EDGE_FALLING, (void*)&button_callback);

    while (running);
    
    printf("Cleaning up");
    fbuff_deinit(fbuff_dev);
    return 0;
}
