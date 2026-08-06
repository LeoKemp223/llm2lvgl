// Controls: 6 button(Auto mode, curtain power, living room light power, AC power, settings, fan/airflow), 0 checkbox, 0 switch, 0 slider, 0 dropdown, 0 textarea
#include "web_4b544161_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static lv_obj_t * make_panel(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                             uint32_t color, int32_t radius)
{
    lv_obj_t * obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    return obj;
}

static lv_obj_t * make_label(lv_obj_t * parent, const char * text, int32_t x, int32_t y,
                             int32_t size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    return label;
}

static lv_obj_t * make_icon_label(lv_obj_t * parent, const char * text, int32_t x, int32_t y,
                                  int32_t size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_icon_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    return label;
}

static lv_obj_t * make_round_icon_button(lv_obj_t * parent, int32_t x, int32_t y, int32_t d,
                                         uint32_t color, const char * icon, int32_t icon_size)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, d, d);
    lv_obj_set_style_radius(btn, d / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    lv_obj_t * l = lv_label_create(btn);
    lv_label_set_text(l, icon);
    lv_obj_set_style_text_font(l, ui_icon_font_get(icon_size), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xffffff), 0);
    lv_obj_center(l);
    return btn;
}

static void add_air_conditioner(lv_obj_t * parent)
{
    lv_obj_t * ac = make_panel(parent, 25, 68, 196, 71, 0xdce2ef, 6);
    lv_obj_set_style_border_width(ac, 2, 0);
    lv_obj_set_style_border_color(ac, lv_color_hex(0xaeb7c9), 0);

    make_panel(ac, 21, 10, 134, 2, 0x83a8bc, 0);
    make_panel(ac, 26, 14, 129, 2, 0x83a8bc, 0);

    lv_obj_t * vent = make_panel(ac, 77, 31, 41, 9, 0x151820, 4);
    make_panel(vent, 17, 3, 7, 3, 0x25e071, 2);

    lv_obj_t * glow = make_panel(ac, 20, 55, 156, 10, 0x3f7dff, 5);
    lv_obj_set_style_shadow_width(glow, 6, 0);
    lv_obj_set_style_shadow_opa(glow, LV_OPA_50, 0);
    lv_obj_set_style_shadow_color(glow, lv_color_hex(0x3f7dff), 0);
}

static void add_curtain(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * rail = make_panel(parent, x, y, 116, 4, 0x06070b, 2);
    (void) rail;

    for(int i = 0; i < 9; i++) {
        uint32_t c = (i % 2) ? 0x31425d : 0x1a263d;
        make_panel(parent, x + 3 + i * 6, y + 5, 5, 75, c, 0);
    }

    lv_obj_t * center = make_panel(parent, x + 47, y + 5, 39, 75, 0xdce7f3, 0);
    lv_obj_set_style_bg_grad_color(center, lv_color_hex(0xb7c8d9), 0);
    lv_obj_set_style_bg_grad_dir(center, LV_GRAD_DIR_HOR, 0);

    for(int i = 0; i < 8; i++) {
        uint32_t c = (i % 2) ? 0x31425d : 0x1a263d;
        make_panel(parent, x + 88 + i * 5, y + 5, 4, 75, c, 0);
    }
}

static void add_pendant_lamp(lv_obj_t * parent, int32_t x, int32_t y)
{
    make_panel(parent, x + 62, y, 4, 54, 0x7d7d7d, 2);

    lv_obj_t * shade = make_panel(parent, x + 18, y + 34, 92, 52, 0xdde0e8, 46);
    lv_obj_set_style_bg_grad_color(shade, lv_color_hex(0x7f8490), 0);
    lv_obj_set_style_bg_grad_dir(shade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_clip_corner(shade, true, 0);

    lv_obj_t * cut = make_panel(parent, x + 14, y + 69, 101, 31, 0x333333, 0);
    (void) cut;

    lv_obj_t * bulb = make_panel(parent, x + 42, y + 68, 45, 45, 0xffaa36, 23);
    lv_obj_set_style_shadow_width(bulb, 18, 0);
    lv_obj_set_style_shadow_opa(bulb, LV_OPA_50, 0);
    lv_obj_set_style_shadow_color(bulb, lv_color_hex(0xffa22c), 0);
}

lv_obj_t * web_4b544161_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 702, 704);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x969697), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * shell_shadow = make_panel(screen, 17, 17, 654, 649, 0x111112, 28);
    lv_obj_set_style_shadow_width(shell_shadow, 12, 0);
    lv_obj_set_style_shadow_opa(shell_shadow, LV_OPA_40, 0);
    lv_obj_set_style_shadow_color(shell_shadow, lv_color_hex(0x000000), 0);

    lv_obj_t * shell = make_panel(screen, 24, 22, 642, 635, 0x252526, 26);
    lv_obj_set_style_bg_grad_color(shell, lv_color_hex(0x111113), 0);
    lv_obj_set_style_bg_grad_dir(shell, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(shell, 3, 0);
    lv_obj_set_style_border_color(shell, lv_color_hex(0x3f4044), 0);

    make_panel(shell, 7, 8, 626, 608, 0x1d1d1f, 18);
    make_panel(shell, 12, 615, 618, 13, 0x050506, 5);
    make_panel(screen, 0, 642, 690, 8, 0xffffff, 4);
    lv_obj_set_style_bg_opa(lv_obj_get_child(screen, lv_obj_get_child_count(screen) - 1), LV_OPA_20, 0);

    lv_obj_t * display = make_panel(shell, 41, 61, 566, 537, 0x11121a, 0);
    lv_obj_set_style_clip_corner(display, true, 0);
    g_content_root = display;

    make_panel(display, 0, 274, 566, 54, 0x273449, 0);
    make_panel(display, 0, 327, 566, 86, 0x6f615a, 0);
    make_panel(display, 0, 413, 566, 124, 0x2d4554, 0);
    make_panel(display, 257, 348, 66, 141, 0x10131a, 0);

    make_label(display, "室内26°C", 13, 9, 20, 0xf4f4f5);
    make_icon_label(display, "\xEE\xA2\xB8", 456, 5, 25, 0xf4f4f5);
    make_icon_label(display, "\xEE\x98\xBE", 493, 6, 27, 0xf4f4f5);

    make_label(display, "09 : 41", 21, 85, 56, 0xffffff);
    make_label(display, "3月15日 周四", 27, 151, 20, 0xf0f0f0);
    make_icon_label(display, "\xEE\x9E\x98", 351, 92, 28, 0xf3f3f4);
    make_label(display, "65%", 400, 80, 54, 0xffffff);
    make_label(display, "空气湿度", 437, 151, 20, 0xf0f0f0);

    lv_obj_t * control = make_panel(display, 23, 202, 520, 151, 0x333333, 18);
    lv_obj_set_style_bg_opa(control, 245, 0);

    lv_obj_t * auto_btn = make_round_icon_button(control, 24, 14, 38, 0x2f76ff, "\xEE\x90\xA9", 25);
    lv_obj_add_state(auto_btn, LV_STATE_CHECKED);

    make_icon_label(control, "\xEF\x81\xB6", 77, 19, 25, 0xe6e6e6);
    lv_obj_t * settings_btn = lv_button_create(control);
    lv_obj_set_pos(settings_btn, 125, 17);
    lv_obj_set_size(settings_btn, 31, 31);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(settings_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(settings_btn, 0, LV_PART_MAIN);
    lv_obj_t * settings_icon = lv_label_create(settings_btn);
    lv_label_set_text(settings_icon, "\xEE\xA2\xB8");
    lv_obj_set_style_text_font(settings_icon, ui_icon_font_get(23), 0);
    lv_obj_set_style_text_color(settings_icon, lv_color_hex(0xe6e6e6), 0);
    lv_obj_center(settings_icon);

    lv_obj_t * fan_btn = lv_button_create(control);
    lv_obj_set_pos(fan_btn, 176, 17);
    lv_obj_set_size(fan_btn, 31, 31);
    lv_obj_set_style_bg_opa(fan_btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(fan_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(fan_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(fan_btn, 0, LV_PART_MAIN);
    lv_obj_t * fan_icon = lv_label_create(fan_btn);
    lv_label_set_text(fan_icon, "\xEE\xBF\x98");
    lv_obj_set_style_text_font(fan_icon, ui_icon_font_get(23), 0);
    lv_obj_set_style_text_color(fan_icon, lv_color_hex(0xe6e6e6), 0);
    lv_obj_center(fan_icon);

    make_label(control, "三级风力", 423, 17, 22, 0xe7e7e7);
    make_panel(control, 23, 58, 474, 2, 0x464646, 0);

    add_air_conditioner(control);
    make_label(control, "25°C", 360, 84, 32, 0xffffff);
    make_round_icon_button(control, 456, 76, 44, 0xff8a00, "\xEE\xA2\xAC", 29);

    lv_obj_t * card1 = make_panel(display, 24, 367, 251, 136, 0x333333, 18);
    make_label(card1, "窗帘", 32, 29, 23, 0xffffff);
    make_round_icon_button(card1, 35, 68, 40, 0xff8a00, "\xEE\xA2\xAC", 27);
    add_curtain(card1, 137, 30);

    lv_obj_t * card2 = make_panel(display, 292, 367, 251, 136, 0x333333, 18);
    make_label(card2, "客厅吊灯", 28, 29, 23, 0xffffff);
    make_round_icon_button(card2, 29, 68, 40, 0x29c751, "\xEE\xA2\xAC", 27);
    add_pendant_lamp(card2, 128, 30);

    return screen;
}

lv_obj_t * web_4b544161_page_get_content_root(void)
{
    return g_content_root;
}
