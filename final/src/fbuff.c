#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "../include/fbuff.h"

#define FILL_BY_BOX

#ifndef FILL_BY_BOX
#define FILL_BY_PIXEL
#endif


void find_change_values(fbuff_dev_info_t* fbuff_dev);

fbuff_dev_info_t* fbuff_init(uint32_t iterations) {
    fbuff_dev_info_t* fbuff_dev = malloc(sizeof(fbuff_dev_info_t));
    int fb_fd = open("/dev/fb0", O_RDWR);
    fbuff_dev->num_bb = iterations;
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

    fbuff_dev->original = malloc(fbuff_dev->screensize);
    memcpy(fbuff_dev->original, fbuff_dev->fbp, fbuff_dev->screensize);
    
    fbuff_dev->bb_array = malloc(iterations*sizeof(uint8_t*));
    uint32_t i;
    for (i = 0; i < iterations; i++) {
        *(fbuff_dev->bb_array + i) = malloc(fbuff_dev->screensize);
    }

    // initialize change values
    find_change_values(fbuff_dev);

    //! ONE CHANGES BUFFER AND ONE ORIGINAL BUFFER. Back buffers are created from it, or we just write one or the other to the FBP
    //update all buffers
    for (i=0; i < iterations; i++) {
        fill_back_buffer(fbuff_dev, i);
    }
    
    return fbuff_dev;
}

uint32_t fbuff_deinit(fbuff_dev_info_t* fbuff_dev) {
    uint32_t iterations = fbuff_dev->num_bb;
    uint32_t i;
    for (i = 0; i < iterations; i++) {
        INDEX_BB(fbuff_dev, i);
    }
    free(fbuff_dev->bb_array);
    free(fbuff_dev->change_buffer);
    free(fbuff_dev->original);

    free(fbuff_dev);
    return 0;
}

inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo)
{
    return (r << vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
}

inline void copy_and_change_pixel(int32_t dr, int32_t dg, int32_t db, uint32_t* pixel_src, uint32_t* pixel_dest, struct fb_var_screeninfo vinfo) {
    int32_t red = GET_RED(*pixel_src, vinfo) + dr;
    int32_t blue = GET_BLUE(*pixel_src, vinfo) + db;
    int32_t green = GET_GREEN(*pixel_src, vinfo) + dg;
    *pixel_dest = MAKE_RED(red, vinfo) | MAKE_GREEN(green, vinfo) | MAKE_BLUE(blue, vinfo);
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
    uint32_t cols = fbuff_dev->cols;
    uint32_t row, column, x, y;
    int red, green, blue, total, color;
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

void find_change_values(fbuff_dev_info_t* fbuff_dev) 
{
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    
    uint32_t curr_brightness[rows][cols];
    memset(curr_brightness, 0, rows*cols*sizeof(uint32_t));

    int32_t change_vals[rows][cols];
    memset(change_vals, 0, rows*cols*sizeof(uint32_t));

    find_brightness(fbuff_dev, (uint32_t*)curr_brightness, fbuff_dev->original);
    find_brightness_changes(fbuff_dev, (uint32_t*)curr_brightness, (int32_t*)change_vals);
    update_buffer(fbuff_dev, (int32_t*)change_vals, fbuff_dev->change_buffer);
}
//TODO make this in place so I dont
int32_t find_brightness_changes(fbuff_dev_info_t* fbuff_dev, uint32_t* curr_brightness, int32_t* change) {
    uint32_t row=0, column=0;
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    for (row = 0; row < rows; row++) {
        for (column = 0; column < cols; column++) {
            if (*(curr_brightness + row*cols + column) < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH / 8) {
                *(change + row*cols + column) = 2*MIN_CHANGE;
            }
            else if (*(curr_brightness + row*cols + column) < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH / 2) {
                *(change + row*cols + column) = MIN_CHANGE;
            } else if (*(curr_brightness + row*cols + column) < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH * 2/3){
                *(change + row*cols + column) = MIN_CHANGE;
            } else {
                *(change + row*cols + column) = -2*MIN_CHANGE;
            }
        }
    }
    return 0;
}

uint32_t update_buffer(fbuff_dev_info_t* fbuff_dev, int32_t* change, uint8_t* buffer) {
    uint32_t row=0, column=0;
    // uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
    struct fb_fix_screeninfo finfo = fbuff_dev->finfo;
    uint8_t dred, dgreen, dblue;
    int32_t d, y, x;
    uint32_t color, red, blue, green;
    uint64_t location = 0;

    for (y = 0; y < vinfo.yres; y++) {
        row = y / BOX_HEIGHT;
        for (x = 0; x < vinfo.xres; x++) {
            column = x / BOX_WIDTH;
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
    return 0;
}

// uint32_t update_buffer_thread(fbuff_dev_info_t* fbuff_dev, int32_t* change, uint8_t* buffer, const uint8_t* original) {
//     uint32_t row=0, column=0;
//     // uint32_t rows = fbuff_dev->rows;
//     uint32_t cols = fbuff_dev->cols;
//     struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
//     struct fb_fix_screeninfo finfo = fbuff_dev->finfo;
//     uint8_t dred, dgreen, dblue;
//     int32_t d, y, x;
//     uint32_t color, red, blue, green;
//     uint64_t location = 0;

//     for (y = 0; y < vinfo.yres; y++) {
//         row = y / BOX_HEIGHT;
//         for (x = 0; x < vinfo.xres; x++) {
//             column = x / BOX_WIDTH;
//             location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
//             color = *((uint32_t*)(original + location));
//             red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
//             blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
//             green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
//     }
//     return 0;
// }


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
            newch = ((row_bit & !(iteration & 1)) | (col_bit & iteration & 1)) * (-(*lch));
            *ch = newch;
            *lch = newch ? newch : (*lch);
            // ch++;
            // lch++;
        }
    }
    return 0;
}

// void fill_change_values(fbuff_dev_info_t* fbuff_dev) {
//     // uint8_t* change = fbuff_dev->
//     uint32_t row=0, column=0;
//     // uint32_t rows = fbuff_dev->rows;
//     uint32_t cols = fbuff_dev->cols;
//     struct fb_var_screeninfo vinfo = fbuff_dev->vinfo;
//     struct fb_fix_screeninfo finfo = fbuff_dev->finfo;
//     uint8_t dred, dgreen, dblue;
//     int32_t d, y, x;
//     uint32_t color, red, blue, green;
//     uint64_t location = 0;
//     for (y = 0; y < vinfo.yres; y++) {
//         row = y / BOX_HEIGHT;
//         for (x = 0; x < vinfo.xres; x++) {
//             column = x / BOX_WIDTH;
//             location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
//             color = *((uint32_t*)(original + location));
//             red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
//             blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
//             green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
//             d = *(change + row*cols + column);
//             if ((red + d) & 0x100) { // if doing change causes overflow
//                 if (d < 0) {
//                     dred = -red;
//                 } else {
//                     dred = 0xFF - red;
//                 }
//             } else {
//                 dred = d;
//             }
//             if ((green + d) & 0x100) { // if doing change causes overflow
//                 if (d < 0) {
//                     dgreen = -green;
//                 } else {
//                     dgreen = 0xFF - green;
//                 }
//             } else {
//                 dgreen = d;
//             }
//             if ((blue + d) & 0x100) { // if doing change causes overflow
//                 if (d < 0) {
//                     dblue = -blue;
//                 } else {
//                     dblue = 0xFF - blue;
//                 }
//             } else {
//                 dblue = d;
//             }
//             copy_and_change_pixel(dred, dgreen, dblue,
//                 ((uint32_t*)(original + location)), ((uint32_t*)(buffer + location)), vinfo);
//         }
//     }
// }


void* fill_back_buffer(fbuff_dev_info_t* fbuff_dev, int32_t iteration) {
    // fbuff_back_buffer_info_t* fbuff_bb = (fbuff_back_buffer_info_t*)fbuff_bb_v;
    uint32_t row=0, column=0;
    uint32_t rows = fbuff_dev->rows;
    uint32_t cols = fbuff_dev->cols;
    uint32_t col_split = 0;
    uint32_t row_split = 0;
    uint32_t box_row, box_col;
    // int32_t* change=fbuff_dev->box_changes;
    int32_t* this_change = (int32_t*) malloc(rows*cols*sizeof(int32_t)); //! remove this when i fill_by_box
    iteration++; // ! check this part
    int x;
    int y;
    uint8_t* bb_base = INDEX_BB(fbuff_dev, iteration);

    col_split = cols / (1 << ((iteration >> 1) + (iteration &1)));
    row_split = iteration == 1 ? rows : rows / (1 << (iteration >>1));
    col_split = col_split < 2 ? 2 : col_split;
    row_split = row_split < 2 ? 2 : row_split;
    int32_t do_i_change = 0;
    for (row = 0; row < rows; row++) {
        box_row = (row / row_split);
        for (column = 0; column < cols; column++) {
            box_col = column/col_split;
            do_i_change = ((__builtin_popcount(box_col) ^ __builtin_popcount(box_row)) & 1);
            // *(this_change + row*cols + column) = do_i_change * (*(change + row*cols + column));
            #ifdef FILL_BY_BOX
            uint8_t* dest;
            uint8_t* buffer_to_use = do_i_change ? fbuff_dev->change_buffer : fbuff_dev->original;
            uint64_t offset = 0;
            // set box pixels to the values from the correct buffer
            for(y = row*BOX_HEIGHT; y < (row+1)*BOX_HEIGHT; y++) {
                for(x = column*BOX_WIDTH; x < (column+1) *BOX_WIDTH; x++) {
                    offset = FBUFF_OFFSET(fbuff_dev, x, y);
                    dest = bb_base + offset;
                    *((uint32_t*) dest) = *((uint32_t*)(buffer_to_use + offset));
                }
            }
            #endif
        }
    }
    #ifdef FILL_BY_PIXEL
    for (y = 0; y < vinfo.yres; y++) {
        row = y / BOX_HEIGHT;
        for (x = 0; x < vinfo.xres; x++) {
            column = x / BOX_WIDTH;
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            color = *((uint32_t*)(original + location));
            // fill pixel with the right one from change or original
        }
    }
    #endif
    // call the update_buffer to fill the buffer
    // memcpy(fbuff_bb->back_buffer, fbuff_bb->original, fbuff_bb->fbuff_dev->screensize);
    // update_buffer_thread(fbuff_dev, this_change, INDEX_BB(fbuff_dev, iteration)->back_buffer, fbuff_dev->original);
    free((void*)this_change);
    printf("iter %d done\n", iteration);
    return NULL;
}
