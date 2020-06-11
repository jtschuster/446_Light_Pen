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

#include "include/fbuff.h"
#include "include/cursor.h"

int main() {
    fbuff_dev_info_t* fbuff_dev = fbuff_init(15);
    fbuff_deinit(fbuff_dev);
}