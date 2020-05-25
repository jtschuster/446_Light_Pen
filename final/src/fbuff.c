#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "../include/fbuff.h"




fbuff_dev_info_t* fbuff_init() {
    fbuff_dev_info_t* fbuff_dev = malloc(sizeof(fbuff_dev_info_t));
    int fb_fd = open("/dev/fb0", O_RDWR);

    // Get variable screen information
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbuff_dev->vinfo);
    fbuff_dev->vinfo.grayscale = 0;
    fbuff_dev->vinfo.bits_per_pixel = 32;
    ioctl(fb_fd, FBIOPUT_VSCREENINFO, &fbuff_dev->vinfo);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbuff_dev->vinfo);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbuff_dev->finfo);
    fbuff_dev->screensize = fbuff_dev->vinfo.yres_virtual * fbuff_dev->finfo.line_length;
    fbuff_dev->fbp = mmap(0, fbuff_dev->screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);
    fbuff_dev->rows = fbuff_dev->vinfo.yres / BOX_HEIGHT + !!(fbuff_dev->vinfo.yres % BOX_HEIGHT);
    fbuff_dev->cols = fbuff_dev->vinfo.xres / BOX_WIDTH + !!(fbuff_dev->vinfo.xres % BOX_WIDTH);
    return fbuff_dev;
}

uint32_t fbuff_deinit(fbuff_dev_info_t* fbuff_dev_info) {
    free(fbuff_dev_info);
}

inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo)
{
    return (r << vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
}

inline uint32_t change_pixel(int32_t dr, int32_t dg, int32_t db, uint32_t* pixel, struct fb_var_screeninfo vinfo)
{
    int32_t red = GET_RED(*pixel, vinfo) + dr;
    int32_t blue = GET_BLUE(*pixel, vinfo) + db;
    int32_t green = GET_GREEN(*pixel, vinfo) + dg;
    *pixel = MAKE_RED(red, vinfo) | MAKE_GREEN(green, vinfo) | MAKE_BLUE(blue, vinfo);
    return 0;
}

int32_t find_brightness(fbuff_dev_info_t* fbuff_dev, uint32_t* brightness_array, uint8_t* back_buffer)
{
    struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
    struct fb_fix_screeninfo finfo = fbuff_dev->finfo;
    uint8_t* fbp = back_buffer ? back_buffer : fbuff_dev->fbp;
    int rows = fbuff_dev->rows;
    int cols = fbuff_dev->cols;

    int red, green, blue, total, color, row, column, x, y;
    uint64_t location;
    for (y = 0; y < vinfo.yres; y++) {
        row = y / BOX_HEIGHT;
        for (x = 0; x < vinfo.xres; x++) {
            column = x / BOX_WIDTH;
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            color = *((uint32_t*)(fbp + location));
            red = GET_RED(color, vinfo);
            blue = GET_BLUE(color, vinfo);
            green = GET_GREEN(color, vinfo);
            total = green + blue + red;
            *(brightness_array + row * cols + column) += total;        
        }
    }
    return 0;
}

int32_t find_brightness_changes(fbuff_dev_info_t* fbuff_dev,uint32_t* curr_brightness, uint32_t* change) {
    uint32_t row=0, column=0;
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    for (row = 0; row < rows; row++) {
        for (column = 0; column < cols; column++) {
            if (*(curr_brightness + row*cols + column) < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH / 2) {
                *(change + row*cols + column) = MIN_CHANGE;
            } else if (*(curr_brightness + row*cols + column) < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH * 3/4){
                *(change + row*cols + column) = -2*MIN_CHANGE;
            } else {
                *(change + row*cols + column) = -2*MIN_CHANGE;
            }
        }
    }
    return 0;
}

uint32_t update_buffer(fbuff_dev_info_t* fbuff_dev, int32_t* change, uint8_t* buffer) {
    uint32_t row=0, column=0;
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
    struct fb_fix_screeninfo finfo = fbuff_dev->finfo;
    uint8_t dred, dgreen, dblue;
    int32_t d, y, x;
    uint32_t color, red, blue, green, total;
    uint64_t location = 0;
    for (y = 0; y < vinfo.yres; y++) {
        row = y / BOX_HEIGHT;
        for (x = 0; x < vinfo.xres; x++) {
            column = x / BOX_WIDTH;
//            uint32_t old_location = location;
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            color = *((uint32_t*)(buffer + location));
            red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
            blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
            green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
            d = *(change + row*cols + column);
            if ((red + d) & 0x100) { // if doing change causes overflow
                if (d < 0) {
                    dred = -red;
                } else {
                    dred = 0xFF - red;
                }
            } else {
                dred = d;
            }
            if ((green + d) & 0x100) { // if doing change causes overflow
                if (d < 0) {
                    dgreen = -green;
                } else {
                    dgreen = 0xFF - green;
                }
            } else {
                dgreen = d;
            }
            if ((blue + d) & 0x100) { // if doing change causes overflow
                if (d < 0) {
                    dblue = -blue;
                } else {
                    dblue = 0xFF - blue;
                }
            } else {
                dblue = d;
            }
            change_pixel(dred, dgreen, dblue,
                ((uint32_t*)(buffer + location)), vinfo);
        }
    }
}


//change all every time to be able to change while reading. 
//  eventually multithread it
//  last_rx is 1 if change, 0 if no change
uint32_t update_brightness_changes(fbuff_dev_info_t* fbuff_dev, int32_t iteration, int32_t* change, int32_t* last_change) {
    uint32_t row=0, column=0;
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    uint32_t col_split = 0;
    uint32_t row_split = 0;
    uint32_t row_bit, col_bit;
    int32_t* ch=change;
    int32_t* lch=last_change;
    int32_t newch;
    iteration++;
    col_split = cols / (1 << ((iteration >> 1) + (iteration &1)));
    row_split = iteration == 1 ? rows : rows / (1 << (iteration >>1));
    col_split = col_split < 2 ? 2 : col_split;
    row_split = row_split < 2 ? 2 : row_split;
    
    for (row = 0; row < rows; row++) {
        row_bit = (row / row_split) & 1;
        for (column = 0; column < cols; column++) {
            ch = (change + row*cols + column);
            lch = (last_change + row*cols + column);
            col_bit = ((column / col_split) & 1);
            newch = (row_bit & !(iteration & 1) | col_bit & iteration & 1) * (-(*lch));
            *ch = newch;
            *lch = newch ? newch : (*lch);
            // ch++;
            // lch++;
        }
    }
    return 0;
}

