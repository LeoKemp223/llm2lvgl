#include "stitch_smart_home_panel_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

lv_obj_t * stitch_smart_home_panel_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_coord_t content_width = 384;
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_size(screen, 480, 480);

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
    lv_obj_set_size(content, 384, LV_SIZE_CONTENT);
    lv_obj_center(content);

    lv_obj_t * title = lv_label_create(content);
    lv_label_set_text(title, "Stitch Smart Home Panel");
    lv_obj_set_style_text_font(title, ui_font_get(32), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title, 384);

    lv_obj_t * item_0 = lv_label_create(content);
    lv_label_set_text(item_0, "Good Morning");
    lv_obj_set_style_text_font(item_0, ui_font_get(32), 0);
    lv_obj_set_style_text_color(item_0, lv_color_hex(0x191c1d), 0);
    lv_label_set_long_mode(item_0, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_0, content_width);

    lv_obj_t * item_1 = lv_label_create(content);
    lv_label_set_text(item_1, "08:42 AM");
    lv_obj_set_style_text_font(item_1, ui_font_get(18), 0);
    lv_obj_set_style_text_color(item_1, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_1, content_width);

    lv_obj_t * item_2 = lv_label_create(content);
    lv_label_set_text(item_2, "72°F Sunny");
    lv_obj_set_style_text_font(item_2, ui_font_get(10), 0);
    lv_obj_set_style_text_color(item_2, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_2, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_2, content_width);

    lv_obj_t * item_3 = lv_button_create(content);
    lv_obj_remove_flag(item_3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_3, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_3, 12, 0);
    lv_obj_set_style_pad_ver(item_3, 6, 0);
    lv_obj_set_style_bg_color(item_3, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_3, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_3, 9999, 0);
    lv_obj_set_style_border_width(item_3, 0, 0);
    lv_obj_set_style_shadow_width(item_3, 0, 0);
    lv_obj_t * item_3_label = lv_label_create(item_3);
    lv_label_set_text(item_3_label, "sensors");
    lv_obj_set_style_text_color(item_3_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_3_label, ui_font_get(18), 0);
    lv_obj_center(item_3_label);

    lv_obj_t * item_4 = lv_button_create(content);
    lv_obj_remove_flag(item_4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(item_4, 40, 40);
    lv_obj_set_style_pad_hor(item_4, 12, 0);
    lv_obj_set_style_pad_ver(item_4, 6, 0);
    lv_obj_set_style_bg_color(item_4, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_4, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_4, 9999, 0);
    lv_obj_set_style_border_width(item_4, 0, 0);
    lv_obj_set_style_shadow_width(item_4, 0, 0);
    lv_obj_t * item_4_label = lv_label_create(item_4);
    lv_label_set_text(item_4_label, "add");
    lv_obj_set_style_text_color(item_4_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_4_label, ui_font_get(18), 0);
    lv_obj_center(item_4_label);

    lv_obj_t * item_5 = lv_button_create(content);
    lv_obj_remove_flag(item_5, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(item_5, 40, 40);
    lv_obj_set_style_pad_hor(item_5, 12, 0);
    lv_obj_set_style_pad_ver(item_5, 6, 0);
    lv_obj_set_style_bg_color(item_5, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_5, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_5, 9999, 0);
    lv_obj_set_style_border_width(item_5, 0, 0);
    lv_obj_set_style_shadow_width(item_5, 0, 0);
    lv_obj_t * item_5_label = lv_label_create(item_5);
    lv_label_set_text(item_5_label, "remove");
    lv_obj_set_style_text_color(item_5_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_5_label, ui_font_get(18), 0);
    lv_obj_center(item_5_label);

    lv_obj_t * item_6 = lv_switch_create(content);
    lv_obj_set_size(item_6, 32, 16);
    lv_obj_set_style_bg_color(item_6, lv_color_hex(0xd1d5db), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(item_6, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(item_6, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(item_6, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(item_6, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(item_6, lv_color_hex(0x2563eb), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(item_6, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(item_6, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(item_6, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(item_6, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(item_6, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(item_6, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(item_6, 0, LV_PART_KNOB);
    lv_obj_set_style_radius(item_6, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_add_state(item_6, LV_STATE_CHECKED);

    lv_obj_t * item_7 = lv_label_create(content);
    lv_label_set_text(item_7, "Main Lights");
    lv_obj_set_style_text_font(item_7, ui_font_get(24), 0);
    lv_obj_set_style_text_color(item_7, lv_color_hex(0x1f2937), 0);
    lv_label_set_long_mode(item_7, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_7, content_width);

    lv_obj_t * item_8 = lv_label_create(content);
    lv_label_set_text(item_8, "80% Brightness");
    lv_obj_set_style_text_font(item_8, ui_font_get(11), 0);
    lv_obj_set_style_text_color(item_8, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_8, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_8, content_width);

    lv_obj_t * item_9 = lv_slider_create(content);
    lv_obj_set_size(item_9, 160, 6);
    lv_slider_set_range(item_9, 0, 100);
    lv_slider_set_value(item_9, 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(item_9, lv_color_hex(0xe5e7eb), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(item_9, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(item_9, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(item_9, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(item_9, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(item_9, lv_color_hex(0x2563eb), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(item_9, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(item_9, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(item_9, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(item_9, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(item_9, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(item_9, 0, LV_PART_KNOB);

    lv_obj_t * item_10 = lv_label_create(content);
    lv_label_set_text(item_10, "Security");
    lv_obj_set_style_text_font(item_10, ui_font_get(24), 0);
    lv_obj_set_style_text_color(item_10, lv_color_hex(0x1f2937), 0);
    lv_label_set_long_mode(item_10, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_10, content_width);

    lv_obj_t * item_11 = lv_label_create(content);
    lv_label_set_text(item_11, "All sensors active");
    lv_obj_set_style_text_font(item_11, ui_font_get(11), 0);
    lv_obj_set_style_text_color(item_11, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_11, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_11, content_width);

    lv_obj_t * item_12 = lv_button_create(content);
    lv_obj_remove_flag(item_12, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_12, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_12, 12, 0);
    lv_obj_set_style_pad_ver(item_12, 6, 0);
    lv_obj_set_style_bg_color(item_12, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_12, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_12, 8, 0);
    lv_obj_set_style_border_width(item_12, 0, 0);
    lv_obj_set_style_shadow_width(item_12, 0, 0);
    lv_obj_t * item_12_label = lv_label_create(item_12);
    lv_label_set_text(item_12_label, "SETTINGS");
    lv_obj_set_style_text_color(item_12_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_12_label, ui_font_get(11), 0);
    lv_obj_center(item_12_label);

    lv_obj_t * item_13 = lv_label_create(content);
    lv_label_set_text(item_13, "Living Windows");
    lv_obj_set_style_text_font(item_13, ui_font_get(24), 0);
    lv_obj_set_style_text_color(item_13, lv_color_hex(0x1f2937), 0);
    lv_label_set_long_mode(item_13, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_13, content_width);

    lv_obj_t * item_14 = lv_label_create(content);
    lv_label_set_text(item_14, "Blinds 40% open");
    lv_obj_set_style_text_font(item_14, ui_font_get(11), 0);
    lv_obj_set_style_text_color(item_14, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_14, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_14, content_width);

    lv_obj_t * item_15 = lv_button_create(content);
    lv_obj_remove_flag(item_15, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_15, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_15, 12, 0);
    lv_obj_set_style_pad_ver(item_15, 6, 0);
    lv_obj_set_style_bg_color(item_15, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_15, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_15, 8, 0);
    lv_obj_set_style_border_width(item_15, 0, 0);
    lv_obj_set_style_shadow_width(item_15, 0, 0);
    lv_obj_t * item_15_label = lv_label_create(item_15);
    lv_label_set_text(item_15_label, "OPEN");
    lv_obj_set_style_text_color(item_15_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_15_label, ui_font_get(11), 0);
    lv_obj_center(item_15_label);

    lv_obj_t * item_16 = lv_button_create(content);
    lv_obj_remove_flag(item_16, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_16, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_16, 12, 0);
    lv_obj_set_style_pad_ver(item_16, 6, 0);
    lv_obj_set_style_bg_color(item_16, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_16, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_16, 8, 0);
    lv_obj_set_style_border_width(item_16, 0, 0);
    lv_obj_set_style_shadow_width(item_16, 0, 0);
    lv_obj_t * item_16_label = lv_label_create(item_16);
    lv_label_set_text(item_16_label, "CLOSE");
    lv_obj_set_style_text_color(item_16_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_16_label, ui_font_get(11), 0);
    lv_obj_center(item_16_label);

    lv_obj_t * item_17 = lv_label_create(content);
    lv_label_set_text(item_17, "Quick Scenes");
    lv_obj_set_style_text_font(item_17, ui_font_get(10), 0);
    lv_obj_set_style_text_color(item_17, lv_color_hex(0x4b5563), 0);
    lv_label_set_long_mode(item_17, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(item_17, content_width);

    lv_obj_t * item_18 = lv_button_create(content);
    lv_obj_remove_flag(item_18, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_18, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_18, 16, 0);
    lv_obj_set_style_pad_ver(item_18, 8, 0);
    lv_obj_set_style_bg_color(item_18, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_18, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_18, 9999, 0);
    lv_obj_set_style_border_width(item_18, 0, 0);
    lv_obj_set_style_shadow_width(item_18, 0, 0);
    lv_obj_t * item_18_label = lv_label_create(item_18);
    lv_label_set_text(item_18_label, "Home");
    lv_obj_set_style_text_color(item_18_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_18_label, ui_font_get(18), 0);
    lv_obj_center(item_18_label);

    lv_obj_t * item_19 = lv_button_create(content);
    lv_obj_remove_flag(item_19, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_19, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_19, 16, 0);
    lv_obj_set_style_pad_ver(item_19, 8, 0);
    lv_obj_set_style_bg_color(item_19, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_19, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_19, 9999, 0);
    lv_obj_set_style_border_width(item_19, 0, 0);
    lv_obj_set_style_shadow_width(item_19, 0, 0);
    lv_obj_t * item_19_label = lv_label_create(item_19);
    lv_label_set_text(item_19_label, "Away");
    lv_obj_set_style_text_color(item_19_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_19_label, ui_font_get(18), 0);
    lv_obj_center(item_19_label);

    lv_obj_t * item_20 = lv_button_create(content);
    lv_obj_remove_flag(item_20, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_20, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_20, 16, 0);
    lv_obj_set_style_pad_ver(item_20, 8, 0);
    lv_obj_set_style_bg_color(item_20, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_20, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_20, 9999, 0);
    lv_obj_set_style_border_width(item_20, 0, 0);
    lv_obj_set_style_shadow_width(item_20, 0, 0);
    lv_obj_t * item_20_label = lv_label_create(item_20);
    lv_label_set_text(item_20_label, "Sleep");
    lv_obj_set_style_text_color(item_20_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_20_label, ui_font_get(18), 0);
    lv_obj_center(item_20_label);

    lv_obj_t * item_21 = lv_button_create(content);
    lv_obj_remove_flag(item_21, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_21, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_21, 16, 0);
    lv_obj_set_style_pad_ver(item_21, 8, 0);
    lv_obj_set_style_bg_color(item_21, lv_color_hex(0x0061ff), 0);
    lv_obj_set_style_bg_opa(item_21, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_21, 12, 0);
    lv_obj_set_style_border_width(item_21, 0, 0);
    lv_obj_set_style_shadow_width(item_21, 0, 0);
    lv_obj_t * item_21_label = lv_label_create(item_21);
    lv_label_set_text(item_21_label, "Home");
    lv_obj_set_style_text_color(item_21_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(item_21_label, ui_font_get(18), 0);
    lv_obj_center(item_21_label);

    lv_obj_t * item_22 = lv_button_create(content);
    lv_obj_remove_flag(item_22, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_22, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_22, 16, 0);
    lv_obj_set_style_pad_ver(item_22, 8, 0);
    lv_obj_set_style_bg_color(item_22, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_22, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_22, 12, 0);
    lv_obj_set_style_border_width(item_22, 0, 0);
    lv_obj_set_style_shadow_width(item_22, 0, 0);
    lv_obj_t * item_22_label = lv_label_create(item_22);
    lv_label_set_text(item_22_label, "Climate");
    lv_obj_set_style_text_color(item_22_label, lv_color_hex(0x5e6263), 0);
    lv_obj_set_style_text_font(item_22_label, ui_font_get(18), 0);
    lv_obj_center(item_22_label);

    lv_obj_t * item_23 = lv_button_create(content);
    lv_obj_remove_flag(item_23, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item_23, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_hor(item_23, 16, 0);
    lv_obj_set_style_pad_ver(item_23, 8, 0);
    lv_obj_set_style_bg_color(item_23, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(item_23, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item_23, 12, 0);
    lv_obj_set_style_border_width(item_23, 0, 0);
    lv_obj_set_style_shadow_width(item_23, 0, 0);
    lv_obj_t * item_23_label = lv_label_create(item_23);
    lv_label_set_text(item_23_label, "Lights");
    lv_obj_set_style_text_color(item_23_label, lv_color_hex(0x5e6263), 0);
    lv_obj_set_style_text_font(item_23_label, ui_font_get(18), 0);
    lv_obj_center(item_23_label);

    g_content_root = content;
    return screen;
}

lv_obj_t * stitch_smart_home_panel_page_get_content_root(void)
{
    return g_content_root;
}
