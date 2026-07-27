#include "web_74afc286_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static lv_obj_t * make_label(lv_obj_t * parent, const char * text, int32_t x, int32_t y,
                             int32_t font_size, uint32_t color, lv_text_align_t align)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_font_get(font_size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, align, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static lv_obj_t * make_panel(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius)
{
    lv_obj_t * panel = lv_obj_create(parent);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_radius(panel, radius, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

static lv_obj_t * make_power_button(lv_obj_t * parent, int32_t x, int32_t y, int32_t size, uint32_t color)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_40, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    int32_t ring = size > 32 ? 17 : 15;
    lv_obj_t * icon_ring = lv_obj_create(btn);
    lv_obj_remove_flag(icon_ring, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(icon_ring, ring, ring);
    lv_obj_set_pos(icon_ring, (size - ring) / 2, (size - ring) / 2 + 2);
    lv_obj_set_style_radius(icon_ring, ring / 2, 0);
    lv_obj_set_style_bg_opa(icon_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(icon_ring, 2, 0);
    lv_obj_set_style_border_color(icon_ring, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_pad_all(icon_ring, 0, 0);

    lv_obj_t * icon_line = lv_obj_create(btn);
    lv_obj_remove_flag(icon_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(icon_line, 2, ring / 2 + 2);
    lv_obj_set_pos(icon_line, size / 2 - 1, (size - ring) / 2 - 1);
    lv_obj_set_style_radius(icon_line, 1, 0);
    lv_obj_set_style_bg_color(icon_line, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(icon_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(icon_line, 0, 0);
    lv_obj_set_style_pad_all(icon_line, 0, 0);

    return btn;
}

static void create_display_background(lv_obj_t * root)
{
    lv_obj_set_style_bg_color(root, lv_color_hex(0x141826), 0);
    lv_obj_set_style_bg_grad_color(root, lv_color_hex(0x40505c), 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);

    lv_obj_t * sea = lv_obj_create(root);
    lv_obj_remove_flag(sea, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(sea, 0, 235);
    lv_obj_set_size(sea, 386, 141);
    lv_obj_set_style_bg_color(sea, lv_color_hex(0x213247), 0);
    lv_obj_set_style_bg_grad_color(sea, lv_color_hex(0x4d5960), 0);
    lv_obj_set_style_bg_grad_dir(sea, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(sea, 170, 0);
    lv_obj_set_style_border_width(sea, 0, 0);
    lv_obj_set_style_radius(sea, 0, 0);
    lv_obj_set_style_pad_all(sea, 0, 0);

    lv_obj_t * horizon = lv_obj_create(root);
    lv_obj_remove_flag(horizon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(horizon, 0, 210);
    lv_obj_set_size(horizon, 386, 42);
    lv_obj_set_style_bg_color(horizon, lv_color_hex(0x775f55), 0);
    lv_obj_set_style_bg_grad_color(horizon, lv_color_hex(0x202838), 0);
    lv_obj_set_style_bg_grad_dir(horizon, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(horizon, 115, 0);
    lv_obj_set_style_border_width(horizon, 0, 0);
    lv_obj_set_style_radius(horizon, 0, 0);
    lv_obj_set_style_pad_all(horizon, 0, 0);

    lv_obj_t * rock = lv_obj_create(root);
    lv_obj_remove_flag(rock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rock, 122, 293);
    lv_obj_set_size(rock, 74, 38);
    lv_obj_set_style_radius(rock, 22, 0);
    lv_obj_set_style_bg_color(rock, lv_color_hex(0x11161b), 0);
    lv_obj_set_style_bg_opa(rock, 170, 0);
    lv_obj_set_style_border_width(rock, 0, 0);
}

static void create_air_conditioner(lv_obj_t * card)
{
    lv_obj_t * ac = lv_obj_create(card);
    lv_obj_remove_flag(ac, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ac, 25, 65);
    lv_obj_set_size(ac, 137, 48);
    lv_obj_set_style_radius(ac, 5, 0);
    lv_obj_set_style_bg_color(ac, lv_color_hex(0xe9eef7), 0);
    lv_obj_set_style_bg_grad_color(ac, lv_color_hex(0xc6ccd8), 0);
    lv_obj_set_style_bg_grad_dir(ac, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(ac, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ac, 1, 0);
    lv_obj_set_style_border_color(ac, lv_color_hex(0x9da4b2), 0);
    lv_obj_set_style_pad_all(ac, 0, 0);

    lv_obj_t * top_line = lv_obj_create(ac);
    lv_obj_remove_flag(top_line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(top_line, 14, 5);
    lv_obj_set_size(top_line, 108, 2);
    lv_obj_set_style_bg_color(top_line, lv_color_hex(0x93aabc), 0);
    lv_obj_set_style_bg_opa(top_line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top_line, 0, 0);
    lv_obj_set_style_radius(top_line, 0, 0);

    lv_obj_t * vent = lv_obj_create(ac);
    lv_obj_remove_flag(vent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(vent, 53, 22);
    lv_obj_set_size(vent, 31, 6);
    lv_obj_set_style_radius(vent, 3, 0);
    lv_obj_set_style_bg_color(vent, lv_color_hex(0x12151d), 0);
    lv_obj_set_style_bg_opa(vent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vent, 1, 0);
    lv_obj_set_style_border_color(vent, lv_color_hex(0x434b5a), 0);

    lv_obj_t * dot = lv_obj_create(vent);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(dot, 13, 1);
    lv_obj_set_size(dot, 5, 3);
    lv_obj_set_style_radius(dot, 2, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x18dd70), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);

    lv_obj_t * glow = lv_obj_create(ac);
    lv_obj_remove_flag(glow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(glow, 13, 38);
    lv_obj_set_size(glow, 111, 7);
    lv_obj_set_style_radius(glow, 4, 0);
    lv_obj_set_style_bg_color(glow, lv_color_hex(0x2872ff), 0);
    lv_obj_set_style_bg_opa(glow, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(glow, 7, 0);
    lv_obj_set_style_shadow_color(glow, lv_color_hex(0x2e7cff), 0);
    lv_obj_set_style_shadow_opa(glow, LV_OPA_80, 0);
    lv_obj_set_style_border_width(glow, 0, 0);
}

static void create_curtain(lv_obj_t * card)
{
    lv_obj_t * curtain = lv_obj_create(card);
    lv_obj_remove_flag(curtain, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(curtain, 95, 30);
    lv_obj_set_size(curtain, 65, 52);
    lv_obj_set_style_bg_color(curtain, lv_color_hex(0x172033), 0);
    lv_obj_set_style_bg_opa(curtain, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(curtain, 1, 0);
    lv_obj_set_style_border_color(curtain, lv_color_hex(0x0a0d14), 0);
    lv_obj_set_style_radius(curtain, 2, 0);
    lv_obj_set_style_pad_all(curtain, 0, 0);

    for(int i = 0; i < 6; i++) {
        lv_obj_t * fold = lv_obj_create(curtain);
        lv_obj_remove_flag(fold, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(fold, 3 + i * 6, 2);
        lv_obj_set_size(fold, 3, 48);
        lv_obj_set_style_bg_color(fold, lv_color_hex(0x34435d), 0);
        lv_obj_set_style_bg_opa(fold, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(fold, 0, 0);
        lv_obj_set_style_radius(fold, 0, 0);
    }

    lv_obj_t * open = lv_obj_create(curtain);
    lv_obj_remove_flag(open, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(open, 28, 2);
    lv_obj_set_size(open, 24, 48);
    lv_obj_set_style_bg_color(open, lv_color_hex(0xf2eee7), 0);
    lv_obj_set_style_bg_grad_color(open, lv_color_hex(0xcfd2d8), 0);
    lv_obj_set_style_bg_grad_dir(open, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(open, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(open, 0, 0);
    lv_obj_set_style_radius(open, 1, 0);
}

static void create_lamp(lv_obj_t * card)
{
    lv_obj_t * cord = lv_obj_create(card);
    lv_obj_remove_flag(cord, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(cord, 125, 22);
    lv_obj_set_size(cord, 2, 36);
    lv_obj_set_style_bg_color(cord, lv_color_hex(0xa7a7a7), 0);
    lv_obj_set_style_bg_opa(cord, LV_OPA_40, 0);
    lv_obj_set_style_border_width(cord, 0, 0);

    lv_obj_t * shade = lv_obj_create(card);
    lv_obj_remove_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(shade, 94, 48);
    lv_obj_set_size(shade, 68, 34);
    lv_obj_set_style_radius(shade, 36, 0);
    lv_obj_set_style_bg_color(shade, lv_color_hex(0xf7f7fb), 0);
    lv_obj_set_style_bg_grad_color(shade, lv_color_hex(0x9da1ab), 0);
    lv_obj_set_style_bg_grad_dir(shade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shade, 1, 0);
    lv_obj_set_style_border_color(shade, lv_color_hex(0xbec2ca), 0);

    lv_obj_t * cut = lv_obj_create(card);
    lv_obj_remove_flag(cut, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(cut, 92, 75);
    lv_obj_set_size(cut, 73, 18);
    lv_obj_set_style_bg_color(cut, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(cut, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cut, 0, 0);
    lv_obj_set_style_radius(cut, 0, 0);

    lv_obj_t * bulb = lv_obj_create(card);
    lv_obj_remove_flag(bulb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bulb, 111, 78);
    lv_obj_set_size(bulb, 34, 23);
    lv_obj_set_style_radius(bulb, 17, 0);
    lv_obj_set_style_bg_color(bulb, lv_color_hex(0xffad37), 0);
    lv_obj_set_style_bg_opa(bulb, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(bulb, 16, 0);
    lv_obj_set_style_shadow_color(bulb, lv_color_hex(0xffa21d), 0);
    lv_obj_set_style_shadow_opa(bulb, LV_OPA_60, 0);
    lv_obj_set_style_border_width(bulb, 0, 0);
}

lv_obj_t * web_74afc286_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 480, 480);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf4f6f8), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * device = lv_obj_create(screen);
    lv_obj_remove_flag(device, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(device, 18, 13);
    lv_obj_set_size(device, 444, 450);
    lv_obj_set_style_radius(device, 14, 0);
    lv_obj_set_style_bg_color(device, lv_color_hex(0x151515), 0);
    lv_obj_set_style_bg_grad_color(device, lv_color_hex(0x050505), 0);
    lv_obj_set_style_bg_grad_dir(device, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(device, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(device, 2, 0);
    lv_obj_set_style_border_color(device, lv_color_hex(0x686868), 0);
    lv_obj_set_style_shadow_width(device, 8, 0);
    lv_obj_set_style_shadow_color(device, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(device, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(device, 0, 0);

    lv_obj_t * bezel_highlight = lv_obj_create(screen);
    lv_obj_remove_flag(bezel_highlight, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bezel_highlight, 24, 19);
    lv_obj_set_size(bezel_highlight, 432, 438);
    lv_obj_set_style_radius(bezel_highlight, 11, 0);
    lv_obj_set_style_bg_opa(bezel_highlight, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bezel_highlight, 1, 0);
    lv_obj_set_style_border_color(bezel_highlight, lv_color_hex(0x333333), 0);
    lv_obj_set_style_pad_all(bezel_highlight, 0, 0);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 47, 60);
    lv_obj_set_size(content, 386, 376);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    create_display_background(content);

    make_label(content, "室内26°C", 8, 6, 13, 0xf0f2f5, LV_TEXT_ALIGN_LEFT);

    make_label(content, "◎", 331, 6, 18, 0xf2f2f2, LV_TEXT_ALIGN_CENTER);
    make_label(content, ")))", 356, 6, 14, 0xf2f2f2, LV_TEXT_ALIGN_CENTER);

    lv_obj_t * time = make_label(content, "09 : 41", 15, 54, 39, 0xffffff, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_style_text_letter_space(time, 1, 0);
    lv_obj_set_style_shadow_width(time, 4, 0);
    lv_obj_set_style_shadow_color(time, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(time, LV_OPA_60, 0);

    make_label(content, "3月15日 周四", 19, 105, 16, 0xe9edf4, LV_TEXT_ALIGN_LEFT);

    make_label(content, "♢", 254, 53, 27, 0xf7f8fa, LV_TEXT_ALIGN_CENTER);
    lv_obj_t * humidity = make_label(content, "65%", 286, 50, 38, 0xffffff, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_style_shadow_width(humidity, 4, 0);
    lv_obj_set_style_shadow_color(humidity, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(humidity, LV_OPA_70, 0);
    make_label(content, "空气湿度", 300, 104, 17, 0xe9edf4, LV_TEXT_ALIGN_LEFT);

    lv_obj_t * hvac = make_panel(content, 17, 136, 354, 108, 15);
    lv_obj_set_style_bg_color(hvac, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(hvac, 238, 0);

    lv_obj_t * mode_auto = lv_button_create(hvac);
    lv_obj_set_pos(mode_auto, 16, 12);
    lv_obj_set_size(mode_auto, 27, 27);
    lv_obj_set_style_radius(mode_auto, 14, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mode_auto, lv_color_hex(0x2d74ff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mode_auto, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(mode_auto, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(mode_auto, 7, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(mode_auto, lv_color_hex(0x2d74ff), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(mode_auto, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mode_auto, 0, LV_PART_MAIN);
    lv_obj_t * auto_label = lv_label_create(mode_auto);
    lv_label_set_text(auto_label, "A");
    lv_obj_set_style_text_font(auto_label, ui_font_get(15), 0);
    lv_obj_set_style_text_color(auto_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(auto_label);

    make_label(hvac, "*", 60, 12, 18, 0xdce2e8, LV_TEXT_ALIGN_CENTER);
    make_label(hvac, "o", 96, 13, 17, 0xdce2e8, LV_TEXT_ALIGN_CENTER);
    make_label(hvac, "~", 132, 12, 20, 0xdce2e8, LV_TEXT_ALIGN_CENTER);
    make_label(hvac, "三级风力", 288, 15, 16, 0xe7e7e7, LV_TEXT_ALIGN_RIGHT);

    lv_obj_t * divider = lv_obj_create(hvac);
    lv_obj_remove_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(divider, 14, 46);
    lv_obj_set_size(divider, 326, 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0x464646), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);

    create_air_conditioner(hvac);

    lv_obj_t * temp = make_label(hvac, "25°C", 245, 74, 25, 0xffffff, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_style_shadow_width(temp, 3, 0);
    lv_obj_set_style_shadow_color(temp, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(temp, LV_OPA_50, 0);
    make_power_button(hvac, 314, 63, 33, 0xff920e);

    lv_obj_t * curtain_card = make_panel(content, 17, 251, 171, 96, 15);
    lv_obj_set_style_bg_color(curtain_card, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(curtain_card, 240, 0);
    make_label(curtain_card, "窗帘", 23, 23, 18, 0xf1f1f1, LV_TEXT_ALIGN_LEFT);
    make_power_button(curtain_card, 23, 52, 30, 0xff970f);
    create_curtain(curtain_card);

    lv_obj_t * lamp_card = make_panel(content, 202, 251, 171, 96, 15);
    lv_obj_set_style_bg_color(lamp_card, lv_color_hex(0x303030), 0);
    lv_obj_set_style_bg_opa(lamp_card, 240, 0);
    make_label(lamp_card, "客厅吊灯", 24, 23, 18, 0xf1f1f1, LV_TEXT_ALIGN_LEFT);
    make_power_button(lamp_card, 24, 52, 30, 0x30c94b);
    create_lamp(lamp_card);

    lv_obj_t * bottom_gloss = lv_obj_create(screen);
    lv_obj_remove_flag(bottom_gloss, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bottom_gloss, 23, 435);
    lv_obj_set_size(bottom_gloss, 434, 25);
    lv_obj_set_style_radius(bottom_gloss, 11, 0);
    lv_obj_set_style_bg_color(bottom_gloss, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(bottom_gloss, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_gloss, 0, 0);

    g_content_root = content;
    return screen;
}

lv_obj_t * web_74afc286_page_get_content_root(void)
{
    return g_content_root;
}
