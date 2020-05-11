#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define BOX_WIDTH 5
#define BOX_HEIGHT 5
#define MIN_CHANGE 0x0a

#define GET_RED(pixel, vinfo) ((pixel & (0xFF << vinfo.red.offset)) >> vinfo.red.offset)
#define GET_GREEN(pixel, vinfo) ((pixel & (0xFF << vinfo.green.offset)) >> vinfo.green.offset)
#define GET_BLUE(pixel, vinfo) ((pixel & (0xFF << vinfo.blue.offset)) >> vinfo.blue.offset)

#define MAKE_RED(value, vinfo) ((value & 0xFF) << vinfo.red.offset)
#define MAKE_GREEN(value, vinfo) ((value & 0xFF) << vinfo.green.offset)
#define MAKE_BLUE(value, vinfo) ((value & 0xFF) << vinfo.blue.offset)

uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo)
{
    return (r << vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
}
uint32_t change_pixel(int32_t dr, int32_t dg, int32_t db, uint32_t* pixel,
    struct fb_var_screeninfo vinfo)
{
    int32_t red = GET_RED(*pixel, vinfo) + dr;
    int32_t blue = GET_BLUE(*pixel, vinfo) + db;
    int32_t green = GET_GREEN(*pixel, vinfo) + dg;
    *pixel = MAKE_RED(red, vinfo) | MAKE_GREEN(green, vinfo) | MAKE_BLUE(blue, vinfo);
}
int32_t find_brightness(uint8_t* fbp, struct fb_var_screeninfo vinfo, struct fb_fix_screeninfo finfo, uint32_t* brightness_array)
{
    int rows = vinfo.yres / BOX_HEIGHT + !!(vinfo.yres % BOX_HEIGHT);
    int cols = vinfo.xres / BOX_WIDTH + !!(vinfo.xres % BOX_WIDTH);

    int red, green, blue, total, color, location, row, column, x, y;
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
            *(brightness_array + row * rows + column) += total;
        }
    }
    return 0;
}

int main()
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    int fb_fd = open("/dev/fb0", O_RDWR);

    // Get variable screen information
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    vinfo.grayscale = 0;
    vinfo.bits_per_pixel = 32;
    ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
    ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
    long screensize = vinfo.yres_virtual * finfo.line_length;
    uint8_t* fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd,
        (off_t)0);
    uint8_t* original = malloc(screensize);
    memcpy(original, fbp, screensize);
    uint8_t* back_buffer = malloc(screensize);
    memcpy(back_buffer, fbp, screensize);
    int x, y;

    // dividend plus 1 if remainder
    int rows = vinfo.yres / BOX_HEIGHT + !!(vinfo.yres % BOX_HEIGHT);
    int cols = vinfo.xres / BOX_WIDTH + !!(vinfo.xres % BOX_WIDTH);

    // index with [x][y]
    // msb indicated
    uint32_t curr_brightness[cols][rows];
    uint32_t column = 0;
    uint32_t row = 0;
    uint64_t location;
    uint32_t color, red, blue, green, total;
    int32_t change[cols][rows];
    find_brightness(back_buffer, vinfo, finfo, (uint32_t *)curr_brightness);
//    for (x = 0; x < vinfo.xres; x++) {
//        column = x / BOX_WIDTH;
//        for (y = 0; y < vinfo.yres; y++) {
//            row = y / BOX_HEIGHT;
//            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
//            color = *((uint32_t*)(back_buffer + location));
//            red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
//            blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
//            green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
//            total = green + blue + red;
//            curr_brightness[column][row] += total;
//
//        }
//    }
        for (x = 0; x < vinfo.xres; x++) {
            column = x / BOX_WIDTH;
    for (y = 0; y < vinfo.yres; y++) {
        row = y / BOX_HEIGHT;
            if (curr_brightness[column][row] < 0xFF * 3 * BOX_HEIGHT * BOX_WIDTH / 2) {
                change[column][row] = MIN_CHANGE;
            } else {
                change[column][row] = -MIN_CHANGE;
            }
        }
    }
    uint8_t dred, dgreen, dblue;
    int32_t d;
    for (x = 0; x < vinfo.xres; x++) {
        column = x / BOX_WIDTH;
        for (y = 0; y < vinfo.yres; y++) {
            row = y / BOX_HEIGHT;
            uint32_t old_location = location;
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            color = *((uint32_t*)(back_buffer + location));
            red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
            blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
            green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
            d = change[column][row];
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

            // uint32_t dred2 = ((red + d) & 0x100) ? (red * (-1 * (d >> 31))) + ((-1) * (0xFF - red) * !(d >> 31))
            //     : d;
            // if (dred != dred2)
            //     printf("yeah that ternary doesn't work");
            change_pixel(dred, dgreen, dblue,
                ((uint32_t*)(back_buffer + location)), vinfo);

            // *((uint32_t*)(back_buffer + location)) += pixel_color(dred, dgreen, dblue, &vinfo);
        }
    }
    memcpy(fbp, back_buffer, screensize);
    sleep(3);
    memcpy(fbp, original, screensize);
    free(original);
    return 0;
}
