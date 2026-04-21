#ifndef PAGE_REGISTRY_H
#define PAGE_REGISTRY_H

#include <stddef.h>

#include "lvgl.h"

typedef lv_obj_t * (*m1_page_create_fn_t)(void);
typedef lv_obj_t * (*m1_page_content_root_fn_t)(void);

typedef struct {
    const char * id;
    const char * name;
    m1_page_create_fn_t create;
    m1_page_content_root_fn_t get_content_root;
} m1_page_descriptor_t;

const m1_page_descriptor_t * m1_page_default(void);
const m1_page_descriptor_t * m1_page_find(const char * id);
const m1_page_descriptor_t * m1_page_list(size_t * count);

#endif
