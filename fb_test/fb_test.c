#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define BOX_WIDTH 10
#define BOX_HEIGHT 10
#define MIN_CHANGE 0x0a

uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b,
    struct fb_var_screeninfo* vinfo)
{
    return (r << vinfo->red.offset) | (g << vinfo->green.offset) | (b << vinfo->blue.offset);
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

    uint8_t* fbp = mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, (off_t)0);

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
    for (x = 0; x < vinfo.xres; x++) {
        column = x / BOX_WIDTH;
        for (y = 0; y < vinfo.yres; y++) {
            row = y / BOX_HEIGHT;
            location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) + (y + vinfo.yoffset) * finfo.line_length;
            color = *((uint32_t*)(fbp + location));
            red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
            blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
            green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
            total = green + blue + red;
            curr_brightness[column][row] += total;
            // int dgreen = 0;
            // int dred = 0;
            // int dblue = 0;
            // if (total > 382)
            // { // half of white
            // 	if (green <= 0x0f)
            // 	{
            // 		dgreen = -green;
            // 	}
            // 	else
            // 	{
            // 		dgreen = -0x0F;
            // 	}
            // 	if (red <= 0x0F)
            // 	{
            // 		dred = -red;
            // 	}
            // 	else
            // 	{
            // 		dred = -0x0F;
            // 	}
            // 	if (blue <= 0x0F)
            // 	{
            // 		dblue = -blue;
            // 	}
            // 	else
            // 	{
            // 		dblue = -0x0F;
            // 	}
        }
    }
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
    uint32_t dred, dgreen, dblue, change;
    for (x = 0; Fx < vinfo.xres; x++) {
        column = x / BOX_WIDTH;
        for (y = 0; y < vinfo.yres; y++) {
            row = y / BOX_HEIGHT;
            color = *((uint32_t*)(fbp + location));
            red = ((0xFF << vinfo.red.offset) & color) >> vinfo.red.offset;
            blue = ((0xFF << vinfo.blue.offset) & color) >> vinfo.blue.offset;
            green = ((0xFF << vinfo.green.offset) & color) >> vinfo.green.offset;
            change = change[column][row];
            if ((red + change) & 0x100) { // if doing change causes overflow
                if (change < 0) {
                    dred = -red;
                } else {
                    dred = 0xFF - red;
                }
            }
            if ((green + change) & 0x100) { // if doing change causes overflow
                if (change < 0) {
                    dgreen = -green;
                } else {
                    dgreen = 0xFF - green;
                }
            }
            if ((blue + change) & 0x100) { // if doing change causes overflow
                if (change < 0) {
                    dblue = -blue;
                } else {
                    dblue = 0xFF - blue;
                }
            }
            uint32_t dred2 = ((red + change) & 0x100) ? (red * (-1 * (change >> 31))) + ((-1) * (0xFF - red) * !(change >> 31)))
                : change;
            if (dred != dred2)
                printf("yeah that ternary doesn't work");
                
            color -= pixel_color(dred, dblue, dred, &vinfo);
            *((uint32_t*)(fbp + location)) = color;
        }
    }

    return 0;
}
