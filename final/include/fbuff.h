#define BOX_WIDTH 3
#define BOX_HEIGHT 3
#define MIN_CHANGE 0x0f

#define GET_RED(pixel, vinfo) ((pixel & (0xFF << vinfo.red.offset)) >> vinfo.red.offset)
#define GET_GREEN(pixel, vinfo) ((pixel & (0xFF << vinfo.green.offset)) >> vinfo.green.offset)
#define GET_BLUE(pixel, vinfo) ((pixel & (0xFF << vinfo.blue.offset)) >> vinfo.blue.offset)

#define MAKE_RED(value, vinfo) ((value & 0xFF) << vinfo.red.offset)
#define MAKE_GREEN(value, vinfo) ((value & 0xFF) << vinfo.green.offset)
#define MAKE_BLUE(value, vinfo) ((value & 0xFF) << vinfo.blue.offset)

#define FBUFF_OFFSET(fbuff_dev, x, y) ((x + fbuff_dev->vinfo.xoffset) * (fbuff_dev->vinfo.bits_per_pixel / 8) + (y + fbuff_dev->vinfo.yoffset) * fbuff_dev->finfo.line_length)

//pointer to base of back buffer
#define INDEX_BB(fbuff_bb, index) (*((uint8_t**)(fbuff_bb->bb_array + index)))


typedef struct fbuff_dev_info {
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    uint8_t* fbp;           // The fb_dev base pointer
    uint8_t* original;      // The original frame buffer contents
    uint8_t* change_buffer;
    uint8_t** bb_array;   // an pointer to an array of back_buffers to be indexed
    // int32_t* box_changes;
    // uint8_t* pixel_changes;
    long screensize;
    uint32_t rows;          // number of boxes along the vertical axis of the screen
    uint32_t cols;          // number of boxes along the horizontal axis of the screen
    uint32_t num_bb;        // number of back buffers. AKA ITERATIONS
} fbuff_dev_info_t;

typedef struct fbuff_fill_bb_thread_wrapper_struct {
    fbuff_dev_info_t* fbuff_dev;
    uint32_t iteration;
} fbuff_fill_bb_thread_wrapper_t;

fbuff_dev_info_t* fbuff_init();

uint32_t fbuff_deinit(fbuff_dev_info_t* fbuff_dev_info);

uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b, struct fb_var_screeninfo* vinfo);

uint32_t change_pixel(int32_t dr, int32_t dg, int32_t db, uint32_t* pixel, struct fb_var_screeninfo vinfo);

int32_t find_brightness(fbuff_dev_info_t* fbuff_dev, uint32_t* brightness_array, uint8_t* back_buffer);

int32_t find_brightness_changes(fbuff_dev_info_t* fbuff_dev,uint32_t* curr_brightness, int32_t* change);

uint32_t update_buffer(fbuff_dev_info_t* fbuff_dev, int32_t* change, uint8_t* buffer);


// fbuff_bb should be of type  fbuff_back_buffer_info_t*
void fill_back_buffer(fbuff_dev_info_t* fbuff_dev, int32_t iteration);

void find_change_values(fbuff_dev_info_t* fbuff_dev);

void* fbuff_fill_bb_thread_wrapper(fbuff_fill_bb_thread_wrapper_t* data);
