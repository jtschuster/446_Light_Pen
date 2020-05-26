#define BOX_WIDTH 3
#define BOX_HEIGHT 3
#define MIN_CHANGE 0x0a

#define GET_RED(pixel, vinfo) ((pixel & (0xFF << vinfo.red.offset)) >> vinfo.red.offset)
#define GET_GREEN(pixel, vinfo) ((pixel & (0xFF << vinfo.green.offset)) >> vinfo.green.offset)
#define GET_BLUE(pixel, vinfo) ((pixel & (0xFF << vinfo.blue.offset)) >> vinfo.blue.offset)

#define MAKE_RED(value, vinfo) ((value & 0xFF) << vinfo.red.offset)
#define MAKE_GREEN(value, vinfo) ((value & 0xFF) << vinfo.green.offset)
#define MAKE_BLUE(value, vinfo) ((value & 0xFF) << vinfo.blue.offset)

typedef struct fbuff_dev_info {
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    long screensize;
    uint8_t* fbp;
    uint32_t rows;
    uint32_t cols;
} fbuff_dev_info_t;

typedef struct fbuff_back_buffer_info{
    fbuff_dev_info_t* fbuff_dev;
    uint8_t* back_buffer;
    const uint8_t* original;
    int32_t* change_vals;
    int32_t iteration;
    // The argument to the thread that has all the info to make a back_buffer
} fbuff_back_buffer_info_t;

fbuff_dev_info_t* fbuff_init();

uint32_t fbuff_deinit(fbuff_dev_info_t* fbuff_dev_info);

uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo);

uint32_t change_pixel(int32_t dr, int32_t dg, int32_t db, uint32_t* pixel, struct fb_var_screeninfo vinfo);

int32_t find_brightness(fbuff_dev_info_t* fbuff_dev, uint32_t* brightness_array, uint8_t* back_buffer);

int32_t find_brightness_changes(fbuff_dev_info_t* fbuff_dev,uint32_t* curr_brightness, uint32_t* change);

uint32_t update_buffer(fbuff_dev_info_t* fbuff_dev, int32_t* change, uint8_t* buffer);

uint32_t update_brightness_changes(fbuff_dev_info_t* fbuff_dev, int32_t last_rx, int32_t* change, int32_t* last_change);

// fbuff_bb should be of type  fbuff_back_buffer_info_t*
void* fill_back_buffer(fbuff_back_buffer_info_t* fbuff_bb);
