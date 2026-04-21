#include "demo_page_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

lv_obj_t * demo_page_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_coord_t content_width = 1184;
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_size(screen, 1280, 800);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(content, 32, 0);
    lv_obj_set_style_pad_row(content, 14, 0);
    lv_obj_set_style_radius(content, 24, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(content, 24, 0);
    lv_obj_set_style_shadow_opa(content, 12, 0);
    lv_obj_set_style_shadow_color(content, lv_color_hex(0xcbd5e1), 0);
    lv_obj_set_size(content, 1184, LV_SIZE_CONTENT);
    lv_obj_center(content);

    lv_obj_t * title = lv_label_create(content);
    lv_label_set_text(title, "Demo Page");
    lv_obj_set_style_text_font(title, ui_font_get(32), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title, 1184);

    lv_obj_t * item_0 = lv_label_create(content);
    lv_label_set_text(item_0, "设备控制面板");
    lv_obj_set_style_text_font(item_0, ui_font_get(32), 0);
    lv_obj_set_style_text_color(item_0, lv_color_hex(0x111827), 0);
    lv_label_set_long_mode(item_0, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_0, content_width);

    lv_obj_t * item_1 = lv_label_create(content);
    lv_label_set_text(item_1, "这是一个用于演示 HTML 到 LVGL 生成流程的页面。");
    lv_obj_set_style_text_font(item_1, ui_font_get(18), 0);
    lv_obj_set_style_text_color(item_1, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_1, content_width);

    lv_obj_t * item_2 = lv_label_create(content);
    lv_label_set_text(item_2, "今日状态");
    lv_obj_set_style_text_font(item_2, ui_font_get(24), 0);
    lv_obj_set_style_text_color(item_2, lv_color_hex(0x1f2937), 0);
    lv_label_set_long_mode(item_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_2, content_width);

    lv_obj_t * item_3 = lv_label_create(content);
    lv_label_set_text(item_3, "所有设备在线，系统运行正常。");
    lv_obj_set_style_text_font(item_3, ui_font_get(18), 0);
    lv_obj_set_style_text_color(item_3, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_3, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_3, content_width);

    lv_obj_t * item_4 = lv_button_create(content);
    lv_obj_remove_flag(item_4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_4, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_4, 12, 0);
    lv_obj_set_style_pad_ver(item_4, 6, 0);
    lv_obj_set_style_bg_color(item_4, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_4, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_4, 18, 0);
    lv_obj_set_style_border_width(item_4, 0, 0);
    lv_obj_set_style_shadow_width(item_4, 0, 0);
    lv_obj_t * item_4_label = lv_label_create(item_4);
    lv_label_set_text(item_4_label, "立即同步");
    lv_obj_set_style_text_color(item_4_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_4_label, ui_font_get(18), 0);
    lv_obj_center(item_4_label);

    lv_obj_t * item_5 = lv_label_create(content);
    lv_label_set_text(item_5, "查看详情");
    lv_obj_set_style_text_font(item_5, ui_font_get(18), 0);
    lv_obj_set_style_text_color(item_5, lv_color_hex(0x2563eb), 0);
    lv_label_set_long_mode(item_5, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_5, content_width);

    g_content_root = content;
    return screen;
}

lv_obj_t * demo_page_page_get_content_root(void)
{
    return g_content_root;
}
