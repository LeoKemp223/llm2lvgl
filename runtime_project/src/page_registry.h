#ifndef PAGE_REGISTRY_H
#define PAGE_REGISTRY_H

#include <stddef.h>

#include "lvgl.h"

typedef lv_obj_t * (*lvgl_page_create_fn_t)(void);
typedef lv_obj_t * (*lvgl_page_content_root_fn_t)(void);

typedef struct {
    const char * id;
    const char * name;
    lvgl_page_create_fn_t create;
    lvgl_page_content_root_fn_t get_content_root;
} lvgl_page_descriptor_t;

const lvgl_page_descriptor_t * lvgl_page_default(void);
const lvgl_page_descriptor_t * lvgl_page_find(const char * id);
const lvgl_page_descriptor_t * lvgl_page_list(size_t * count);

#endif
