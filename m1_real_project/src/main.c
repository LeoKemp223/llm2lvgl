#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl.h"
#include "src/display/lv_display.h"
#include "src/draw/snapshot/lv_snapshot.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_mousewheel.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/libs/lodepng/lodepng.h"

#include "page_registry.h"

#define SCREENSHOT_SETTLE_ITERATIONS 5

static const m1_page_descriptor_t * g_active_page = NULL;

static int get_env_dimension(const char * name, int fallback)
{
    const char * env = getenv(name);
    if(env != NULL && env[0] != '\0') {
        int value = atoi(env);
        if(value >= 64 && value <= 4096) {
            return value;
        }
    }
    return fallback;
}

static int get_settle_iterations(void)
{
    const char * env = getenv("LVGL_SCREENSHOT_SETTLE");
    if(env != NULL && env[0] != '\0') {
        int val = atoi(env);
        if(val >= 1 && val <= 100) {
            return val;
        }
    }
    return SCREENSHOT_SETTLE_ITERATIONS;
}

static uint8_t blend_channel(uint8_t fg, uint8_t bg, uint8_t alpha)
{
    uint16_t mixed = (uint16_t)fg * alpha + (uint16_t)bg * (255U - alpha);
    return (uint8_t)(mixed / 255U);
}

static void draw_buf_to_rgba8888(const lv_draw_buf_t * draw_buf, uint8_t * out_rgba)
{
    const uint8_t * src_row = draw_buf->data;
    uint32_t width = draw_buf->header.w;
    uint32_t height = draw_buf->header.h;
    uint32_t stride = draw_buf->header.stride;
    const uint8_t bg_r = 0xf8;
    const uint8_t bg_g = 0xfa;
    const uint8_t bg_b = 0xfc;

    for(uint32_t y = 0; y < height; ++y) {
        const uint8_t * src = src_row;
        uint8_t * dst = out_rgba + y * width * 4;

        for(uint32_t x = 0; x < width; ++x) {
            uint8_t src_b = src[x * 4 + 0];
            uint8_t src_g = src[x * 4 + 1];
            uint8_t src_r = src[x * 4 + 2];
            uint8_t src_a = src[x * 4 + 3];

            dst[x * 4 + 0] = blend_channel(src_r, bg_r, src_a);
            dst[x * 4 + 1] = blend_channel(src_g, bg_g, src_a);
            dst[x * 4 + 2] = blend_channel(src_b, bg_b, src_a);
            dst[x * 4 + 3] = 255;
        }

        src_row += stride;
    }
}

static int save_snapshot_of(lv_obj_t * target, const char * output_path)
{
    lv_draw_buf_t * snapshot = lv_snapshot_take(target, LV_COLOR_FORMAT_ARGB8888);
    if(snapshot == NULL) {
        fprintf(stderr, "Failed to take LVGL snapshot\n");
        return 1;
    }

    uint32_t width = snapshot->header.w;
    uint32_t height = snapshot->header.h;
    size_t rgba_size = (size_t)width * height * 4;
    uint8_t * rgba = malloc(rgba_size);
    if(rgba == NULL) {
        fprintf(stderr, "Failed to allocate snapshot conversion buffer\n");
        lv_draw_buf_destroy(snapshot);
        return 1;
    }

    draw_buf_to_rgba8888(snapshot, rgba);

    unsigned char * png_data = NULL;
    size_t png_size = 0;
    unsigned error = lodepng_encode32(&png_data, &png_size, rgba, width, height);
    free(rgba);
    lv_draw_buf_destroy(snapshot);

    if(error != 0U) {
        fprintf(stderr, "Failed to encode PNG %s: %u\n", output_path, error);
        return 1;
    }

    FILE * file = fopen(output_path, "wb");
    if(file == NULL) {
        fprintf(stderr, "Failed to open screenshot path %s for writing\n", output_path);
        free(png_data);
        return 1;
    }

    if(fwrite(png_data, 1, png_size, file) != png_size) {
        fprintf(stderr, "Failed to write screenshot bytes to %s\n", output_path);
        fclose(file);
        free(png_data);
        return 1;
    }

    fclose(file);
    free(png_data);

    fprintf(stdout, "Saved screenshot to %s\n", output_path);
    return 0;
}

static int maybe_capture_and_exit(void)
{
    const char * viewport_output_path = getenv("LVGL_SCREENSHOT_OUT");
    const char * content_output_path = getenv("LVGL_SCREENSHOT_FULL_OUT");
    if((viewport_output_path == NULL || viewport_output_path[0] == '\0') &&
       (content_output_path == NULL || content_output_path[0] == '\0')) {
        return -1;
    }

    for(uint32_t i = 0; i < (uint32_t)get_settle_iterations(); ++i) {
        lv_timer_handler();
        lv_delay_ms(16);
    }

    if(viewport_output_path != NULL && viewport_output_path[0] != '\0') {
        int viewport_result = save_snapshot_of(lv_screen_active(), viewport_output_path);
        if(viewport_result != 0) {
            return viewport_result;
        }
    }

    if(content_output_path != NULL && content_output_path[0] != '\0') {
        lv_obj_t * content_root = NULL;
        if(g_active_page != NULL && g_active_page->get_content_root != NULL) {
            content_root = g_active_page->get_content_root();
        }

        if(content_root == NULL) {
            fprintf(stderr, "Active page content root is not available for full screenshot\n");
            return 1;
        }

        return save_snapshot_of(content_root, content_output_path);
    }

    return 0;
}

static lv_display_t * create_display(void)
{
    int width = get_env_dimension("M1_VIEWPORT_WIDTH", 1280);
    int height = get_env_dimension("M1_VIEWPORT_HEIGHT", 800);
    lv_display_t * display = lv_sdl_window_create(width, height);
    if(display == NULL) {
        return NULL;
    }

    lv_group_set_default(lv_group_create());

    lv_indev_t * mouse = lv_sdl_mouse_create();
    lv_indev_set_display(mouse, display);
    lv_indev_set_group(mouse, lv_group_get_default());

    lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, display);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t * keyboard = lv_sdl_keyboard_create();
    lv_indev_set_display(keyboard, display);
    lv_indev_set_group(keyboard, lv_group_get_default());

    return display;
}

static void print_available_pages(void)
{
    const m1_page_descriptor_t * pages = NULL;
    size_t count = 0;
    size_t i;

    pages = m1_page_list(&count);
    fprintf(stdout, "Available pages:\n");
    for(i = 0; i < count; ++i) {
        fprintf(stdout, "  %s\t%s\n", pages[i].id, pages[i].name);
    }
}

static const char * resolve_requested_page_id(int argc, char ** argv)
{
    int i;
    const char * env_page = getenv("M1_PAGE");

    for(i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--page") == 0 && i + 1 < argc) {
            return argv[i + 1];
        }

        if(strcmp(argv[i], "--list-pages") == 0) {
            return NULL;
        }
    }

    return env_page;
}

int main(int argc, char ** argv)
{
    const char * requested_page_id = NULL;
    lv_obj_t * screen = NULL;
#ifndef WIN32
    /* Avoid noisy DBus warnings on some Linux desktop setups. */
    setenv("DBUS_FATAL_WARNINGS", "0", 1);
#endif

    if(argc > 1 && strcmp(argv[1], "--list-pages") == 0) {
        print_available_pages();
        return 0;
    }

    lv_init();

    lv_display_t * display = create_display();
    if(display == NULL) {
        return 1;
    }

    lv_display_set_theme(display, NULL);

    requested_page_id = resolve_requested_page_id(argc, argv);
    g_active_page = m1_page_find(requested_page_id);
    if(g_active_page == NULL) {
        fprintf(stderr, "Unknown page id: %s\n", requested_page_id);
        print_available_pages();
        return 1;
    }

    screen = g_active_page->create();
    lv_screen_load(screen);

    int screenshot_result = maybe_capture_and_exit();
    if(screenshot_result >= 0) {
        return screenshot_result;
    }

    while(true) {
        lv_timer_handler();
        lv_delay_ms(5);
    }

    return 0;
}
