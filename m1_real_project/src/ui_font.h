#ifndef UI_FONT_H
#define UI_FONT_H

#include "lvgl.h"

const lv_font_t * ui_font_get(int size);
void ui_font_cleanup(void);

#endif
