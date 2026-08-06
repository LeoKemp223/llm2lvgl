// Controls: 12 buttons(系统设置, 自动模式, 制冷模式, 空调设置, 送风模式, 三级风力, 空调电源, 空调卡片, 窗帘电源, 窗帘卡片, 客厅吊灯电源, 客厅吊灯卡片)
#include "web_f1249c8a_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static lv_obj_t * g_mode_buttons[4];
static lv_obj_t * g_mode_icons[4];
static lv_obj_t * g_fan_label = NULL;
static lv_obj_t * g_ac_temp_label = NULL;
static lv_obj_t * g_ac_power_icon = NULL;
static lv_obj_t * g_curtain_power_icon = NULL;
static lv_obj_t * g_light_power_icon = NULL;
static lv_obj_t * g_curtain_panel = NULL;
static lv_obj_t * g_light_glow = NULL;
static lv_obj_t * g_status_label = NULL;

static int g_selected_mode = 0;
static int g_fan_level = 3;
static bool g_ac_on = true;
static bool g_curtain_on = true;
static bool g_light_on = true;

static lv_obj_t * make_label(lv_obj_t * parent, const char * text, int32_t x, int32_t y, int32_t w,
                             int32_t font_size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    if(w > 0) {
        lv_obj_set_width(label, w);
    }
    lv_obj_set_style_text_font(label, ui_font_get(font_size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static lv_obj_t * make_icon_label(lv_obj_t * parent, const char * icon, int32_t x, int32_t y, int32_t size,
                                  uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, icon);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_icon_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static void style_flat_button(lv_obj_t * obj, uint32_t color, lv_opa_t opa, int32_t radius)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, opa, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x4a4a4a), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_50, LV_PART_MAIN | LV_STATE_PRESSED);
}

static lv_obj_t * make_icon_button(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                                   uint32_t color, const char * icon, int32_t icon_size,
                                   uint32_t icon_color, lv_event_cb_t cb)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    style_flat_button(btn, color, LV_OPA_COVER, LV_MIN(w, h) / 2);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    if(cb) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_font(label, ui_icon_font_get(icon_size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(icon_color), 0);
    lv_obj_center(label);
    return btn;
}

static void update_mode_buttons(void)
{
    static const uint32_t off_colors[4] = {0xd8dee8, 0xd8dee8, 0xd8dee8, 0xd8dee8};

    for(int i = 0; i < 4; i++) {
        if(i == g_selected_mode) {
            lv_obj_set_style_bg_color(g_mode_buttons[i], lv_color_hex(0x2478ff), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(g_mode_buttons[i], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_text_color(g_mode_icons[i], lv_color_hex(0xffffff), 0);
        } else {
            lv_obj_set_style_bg_opa(g_mode_buttons[i], LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_text_color(g_mode_icons[i], lv_color_hex(off_colors[i]), 0);
        }
    }
}

static void mode_clicked_cb(lv_event_t * e)
{
    intptr_t idx = (intptr_t)lv_event_get_user_data(e);
    g_selected_mode = (int)idx;
    update_mode_buttons();

    static const char * names[] = {"自动模式", "制冷模式", "空调设置", "送风模式"};
    lv_label_set_text(g_status_label, names[g_selected_mode]);
}

static void fan_clicked_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    g_fan_level++;
    if(g_fan_level > 5) {
        g_fan_level = 1;
    }

    static const char * fan_text[] = {"", "一级风力", "二级风力", "三级风力", "四级风力", "五级风力"};
    lv_label_set_text(g_fan_label, fan_text[g_fan_level]);
    lv_label_set_text(g_status_label, fan_text[g_fan_level]);
}

static void settings_clicked_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_label_set_text(g_status_label, "系统设置");
}

static void card_clicked_cb(lv_event_t * e)
{
    const char * text = (const char *)lv_event_get_user_data(e);
    if(text) {
        lv_label_set_text(g_status_label, text);
    }
}

static void update_power_visuals(void)
{
    lv_obj_t * ac_btn = lv_obj_get_parent(g_ac_power_icon);
    lv_obj_t * curtain_btn = lv_obj_get_parent(g_curtain_power_icon);
    lv_obj_t * light_btn = lv_obj_get_parent(g_light_power_icon);

    lv_obj_set_style_bg_color(ac_btn, lv_color_hex(g_ac_on ? 0xff9100 : 0x66676a), LV_PART_MAIN);
    lv_obj_set_style_text_color(g_ac_power_icon, lv_color_hex(g_ac_on ? 0xffffff : 0xc8c8c8), 0);
    lv_label_set_text(g_ac_temp_label, g_ac_on ? "25°C" : "--°C");

    lv_obj_set_style_bg_color(curtain_btn, lv_color_hex(g_curtain_on ? 0xff9100 : 0x66676a), LV_PART_MAIN);
    lv_obj_set_style_text_color(g_curtain_power_icon, lv_color_hex(g_curtain_on ? 0xffffff : 0xc8c8c8), 0);
    lv_obj_set_style_bg_color(g_curtain_panel, lv_color_hex(g_curtain_on ? 0xe9eef6 : 0x8791a0), 0);

    lv_obj_set_style_bg_color(light_btn, lv_color_hex(g_light_on ? 0x28c34a : 0x66676a), LV_PART_MAIN);
    lv_obj_set_style_text_color(g_light_power_icon, lv_color_hex(g_light_on ? 0xffffff : 0xc8c8c8), 0);
    lv_obj_set_style_bg_color(g_light_glow, lv_color_hex(g_light_on ? 0xffaa3a : 0x7b756e), 0);
    lv_obj_set_style_shadow_width(g_light_glow, g_light_on ? 22 : 0, 0);
}

static void ac_power_clicked_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    g_ac_on = !g_ac_on;
    update_power_visuals();
    lv_label_set_text(g_status_label, g_ac_on ? "空调已开启" : "空调已关闭");
}

static void curtain_power_clicked_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    g_curtain_on = !g_curtain_on;
    update_power_visuals();
    lv_label_set_text(g_status_label, g_curtain_on ? "窗帘已开启" : "窗帘已关闭");
}

static void light_power_clicked_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    g_light_on = !g_light_on;
    update_power_visuals();
    lv_label_set_text(g_status_label, g_light_on ? "客厅吊灯已开启" : "客厅吊灯已关闭");
}

static void add_seascape_background(lv_obj_t * parent)
{
    lv_obj_t * sky = lv_obj_create(parent);
    lv_obj_remove_flag(sky, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(sky, 0, 0);
    lv_obj_set_size(sky, 565, 535);
    lv_obj_set_style_radius(sky, 0, 0);
    lv_obj_set_style_border_width(sky, 0, 0);
    lv_obj_set_style_bg_color(sky, lv_color_hex(0x11131e), 0);
    lv_obj_set_style_bg_grad_color(sky, lv_color_hex(0x29344b), 0);
    lv_obj_set_style_bg_grad_dir(sky, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(sky, LV_OPA_COVER, 0);

    lv_obj_t * horizon = lv_obj_create(parent);
    lv_obj_remove_flag(horizon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(horizon, 0, 310);
    lv_obj_set_size(horizon, 565, 95);
    lv_obj_set_style_border_width(horizon, 0, 0);
    lv_obj_set_style_radius(horizon, 0, 0);
    lv_obj_set_style_bg_color(horizon, lv_color_hex(0x7a665b), 0);
    lv_obj_set_style_bg_grad_color(horizon, lv_color_hex(0x283649), 0);
    lv_obj_set_style_bg_grad_dir(horizon, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(horizon, LV_OPA_70, 0);

    lv_obj_t * ocean = lv_obj_create(parent);
    lv_obj_remove_flag(ocean, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ocean, 0, 400);
    lv_obj_set_size(ocean, 565, 135);
    lv_obj_set_style_border_width(ocean, 0, 0);
    lv_obj_set_style_radius(ocean, 0, 0);
    lv_obj_set_style_bg_color(ocean, lv_color_hex(0x334c5e), 0);
    lv_obj_set_style_bg_grad_color(ocean, lv_color_hex(0x14202b), 0);
    lv_obj_set_style_bg_grad_dir(ocean, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(ocean, LV_OPA_COVER, 0);

    lv_obj_t * rock = lv_obj_create(parent);
    lv_obj_remove_flag(rock, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rock, 230, 390);
    lv_obj_set_size(rock, 92, 138);
    lv_obj_set_style_radius(rock, 42, 0);
    lv_obj_set_style_border_width(rock, 0, 0);
    lv_obj_set_style_bg_color(rock, lv_color_hex(0x10141b), 0);
    lv_obj_set_style_bg_opa(rock, LV_OPA_90, 0);
}

static void add_ac_illustration(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * ac = lv_obj_create(parent);
    lv_obj_remove_flag(ac, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ac, x, y);
    lv_obj_set_size(ac, 198, 70);
    lv_obj_set_style_radius(ac, 7, 0);
    lv_obj_set_style_border_width(ac, 2, 0);
    lv_obj_set_style_border_color(ac, lv_color_hex(0xa4acba), 0);
    lv_obj_set_style_bg_color(ac, lv_color_hex(0xe7ebf4), 0);
    lv_obj_set_style_bg_grad_color(ac, lv_color_hex(0xcbd2df), 0);
    lv_obj_set_style_bg_grad_dir(ac, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(ac, LV_OPA_COVER, 0);

    lv_obj_t * vent = lv_obj_create(ac);
    lv_obj_remove_flag(vent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(vent, 20, 5);
    lv_obj_set_size(vent, 156, 3);
    lv_obj_set_style_border_width(vent, 0, 0);
    lv_obj_set_style_radius(vent, 1, 0);
    lv_obj_set_style_bg_color(vent, lv_color_hex(0x8fb0c5), 0);

    lv_obj_t * display = lv_obj_create(ac);
    lv_obj_remove_flag(display, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(display, 76, 31);
    lv_obj_set_size(display, 58, 9);
    lv_obj_set_style_radius(display, 5, 0);
    lv_obj_set_style_border_width(display, 1, 0);
    lv_obj_set_style_border_color(display, lv_color_hex(0x5b6672), 0);
    lv_obj_set_style_bg_color(display, lv_color_hex(0x101520), 0);
    lv_obj_set_style_bg_opa(display, LV_OPA_COVER, 0);

    lv_obj_t * led = lv_obj_create(display);
    lv_obj_remove_flag(led, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(led, 27, 2);
    lv_obj_set_size(led, 7, 4);
    lv_obj_set_style_radius(led, 2, 0);
    lv_obj_set_style_border_width(led, 0, 0);
    lv_obj_set_style_bg_color(led, lv_color_hex(0x23df70), 0);

    lv_obj_t * blue = lv_obj_create(ac);
    lv_obj_remove_flag(blue, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(blue, 18, 55);
    lv_obj_set_size(blue, 162, 9);
    lv_obj_set_style_radius(blue, 4, 0);
    lv_obj_set_style_border_width(blue, 0, 0);
    lv_obj_set_style_bg_color(blue, lv_color_hex(0x2c77ff), 0);
    lv_obj_set_style_shadow_width(blue, 8, 0);
    lv_obj_set_style_shadow_color(blue, lv_color_hex(0x5c91ff), 0);
    lv_obj_set_style_shadow_opa(blue, LV_OPA_70, 0);
}

static void add_curtain_illustration(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * rail = lv_obj_create(parent);
    lv_obj_remove_flag(rail, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rail, x - 2, y);
    lv_obj_set_size(rail, 100, 5);
    lv_obj_set_style_border_width(rail, 0, 0);
    lv_obj_set_style_radius(rail, 2, 0);
    lv_obj_set_style_bg_color(rail, lv_color_hex(0x0b0d12), 0);

    for(int i = 0; i < 8; i++) {
        lv_obj_t * fold = lv_obj_create(parent);
        lv_obj_remove_flag(fold, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(fold, x + i * 7, y + 5);
        lv_obj_set_size(fold, 5, 58);
        lv_obj_set_style_border_width(fold, 0, 0);
        lv_obj_set_style_radius(fold, 0, 0);
        lv_obj_set_style_bg_color(fold, lv_color_hex((i % 2) ? 0x31445f : 0x17243a), 0);
    }

    g_curtain_panel = lv_obj_create(parent);
    lv_obj_remove_flag(g_curtain_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(g_curtain_panel, x + 50, y + 6);
    lv_obj_set_size(g_curtain_panel, 38, 57);
    lv_obj_set_style_border_width(g_curtain_panel, 0, 0);
    lv_obj_set_style_radius(g_curtain_panel, 0, 0);
    lv_obj_set_style_bg_color(g_curtain_panel, lv_color_hex(0xe9eef6), 0);
}

static void add_lamp_illustration(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * wire = lv_obj_create(parent);
    lv_obj_remove_flag(wire, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(wire, x + 42, y);
    lv_obj_set_size(wire, 4, 32);
    lv_obj_set_style_border_width(wire, 0, 0);
    lv_obj_set_style_radius(wire, 2, 0);
    lv_obj_set_style_bg_color(wire, lv_color_hex(0x8b8d91), 0);
    lv_obj_set_style_bg_opa(wire, LV_OPA_80, 0);

    g_light_glow = lv_obj_create(parent);
    lv_obj_remove_flag(g_light_glow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(g_light_glow, x + 28, y + 58);
    lv_obj_set_size(g_light_glow, 48, 48);
    lv_obj_set_style_radius(g_light_glow, 24, 0);
    lv_obj_set_style_border_width(g_light_glow, 0, 0);
    lv_obj_set_style_bg_color(g_light_glow, lv_color_hex(0xffaa3a), 0);
    lv_obj_set_style_shadow_width(g_light_glow, 22, 0);
    lv_obj_set_style_shadow_color(g_light_glow, lv_color_hex(0xffaa3a), 0);
    lv_obj_set_style_shadow_opa(g_light_glow, LV_OPA_50, 0);

    lv_obj_t * shade = lv_obj_create(parent);
    lv_obj_remove_flag(shade, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(shade, x, y + 32);
    lv_obj_set_size(shade, 88, 45);
    lv_obj_set_style_radius(shade, 44, 44);
    lv_obj_set_style_border_width(shade, 0, 0);
    lv_obj_set_style_bg_color(shade, lv_color_hex(0xf3f4f8), 0);
    lv_obj_set_style_bg_grad_color(shade, lv_color_hex(0xadb4c1), 0);
    lv_obj_set_style_bg_grad_dir(shade, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(shade, LV_OPA_COVER, 0);

    lv_obj_t * cut = lv_obj_create(parent);
    lv_obj_remove_flag(cut, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(cut, x, y + 73);
    lv_obj_set_size(cut, 88, 20);
    lv_obj_set_style_border_width(cut, 0, 0);
    lv_obj_set_style_radius(cut, 0, 0);
    lv_obj_set_style_bg_color(cut, lv_color_hex(0x303030), 0);
}

lv_obj_t * web_f1249c8a_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 702, 704);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x969697), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * shell = lv_obj_create(screen);
    lv_obj_remove_flag(shell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(shell, 18, 16);
    lv_obj_set_size(shell, 658, 662);
    lv_obj_set_style_radius(shell, 24, 0);
    lv_obj_set_style_border_width(shell, 3, 0);
    lv_obj_set_style_border_color(shell, lv_color_hex(0x202024), 0);
    lv_obj_set_style_bg_color(shell, lv_color_hex(0x171719), 0);
    lv_obj_set_style_bg_grad_color(shell, lv_color_hex(0x3b3b3d), 0);
    lv_obj_set_style_bg_grad_dir(shell, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(shell, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(shell, 10, 0);
    lv_obj_set_style_shadow_color(shell, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_shadow_opa(shell, LV_OPA_40, 0);

    lv_obj_t * content = lv_obj_create(shell);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 48, 68);
    lv_obj_set_size(content, 565, 535);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_clip_corner(content, true, 0);
    add_seascape_background(content);

    lv_obj_t * top_scrim = lv_obj_create(content);
    lv_obj_remove_flag(top_scrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(top_scrim, 0, 0);
    lv_obj_set_size(top_scrim, 565, 260);
    lv_obj_set_style_border_width(top_scrim, 0, 0);
    lv_obj_set_style_radius(top_scrim, 0, 0);
    lv_obj_set_style_bg_color(top_scrim, lv_color_hex(0x11121d), 0);
    lv_obj_set_style_bg_opa(top_scrim, LV_OPA_80, 0);

    make_label(content, "室内26°C", 12, 7, 120, 18, 0xffffff);
    make_label(content, "09 : 41", 20, 82, 260, 50, 0xffffff);
    make_label(content, "3月15日 周四", 26, 145, 160, 20, 0xffffff);

    make_icon_label(content, "\xEE\x9E\x98", 354, 88, 27, 0xf2f4f8);
    make_label(content, "65%", 400, 78, 140, 50, 0xffffff);
    make_label(content, "空气湿度", 441, 146, 94, 20, 0xffffff);

    lv_obj_t * settings_btn = lv_button_create(content);
    lv_obj_set_pos(settings_btn, 478, 4);
    lv_obj_set_size(settings_btn, 34, 34);
    style_flat_button(settings_btn, 0x000000, LV_OPA_TRANSP, 17);
    lv_obj_add_event_cb(settings_btn, settings_clicked_cb, LV_EVENT_CLICKED, NULL);
    make_icon_label(settings_btn, "\xEE\xA2\xB8", 5, 4, 24, 0xffffff);

    make_icon_label(content, "\xEE\x98\xBE", 516, 9, 28, 0xffffff);

    lv_obj_t * ac_card = lv_button_create(content);
    lv_obj_set_pos(ac_card, 20, 198);
    lv_obj_set_size(ac_card, 520, 153);
    style_flat_button(ac_card, 0x323233, LV_OPA_COVER, 18);
    lv_obj_add_event_cb(ac_card, card_clicked_cb, LV_EVENT_CLICKED, "空调详细控制");

    lv_obj_t * ac_div = lv_obj_create(ac_card);
    lv_obj_remove_flag(ac_div, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ac_div, 25, 55);
    lv_obj_set_size(ac_div, 468, 1);
    lv_obj_set_style_border_width(ac_div, 0, 0);
    lv_obj_set_style_bg_color(ac_div, lv_color_hex(0x505052), 0);
    lv_obj_set_style_bg_opa(ac_div, LV_OPA_70, 0);

    static const char * mode_icons[] = {"A", "\xEE\xBF\x98", "\xEE\xA2\xB8", "\xEE\xBF\x98"};
    for(int i = 0; i < 4; i++) {
        g_mode_buttons[i] = lv_button_create(ac_card);
        lv_obj_set_pos(g_mode_buttons[i], 24 + i * 50, 12);
        lv_obj_set_size(g_mode_buttons[i], 34, 34);
        style_flat_button(g_mode_buttons[i], 0x2478ff, i == 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 17);
        lv_obj_add_event_cb(g_mode_buttons[i], mode_clicked_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        g_mode_icons[i] = lv_label_create(g_mode_buttons[i]);
        lv_label_set_text(g_mode_icons[i], mode_icons[i]);
        lv_obj_set_style_text_font(g_mode_icons[i], i == 0 ? ui_font_get(16) : ui_icon_font_get(22), 0);
        lv_obj_set_style_text_color(g_mode_icons[i], lv_color_hex(i == 0 ? 0xffffff : 0xd8dee8), 0);
        lv_obj_center(g_mode_icons[i]);
    }

    lv_obj_t * fan_btn = lv_button_create(ac_card);
    lv_obj_set_pos(fan_btn, 410, 6);
    lv_obj_set_size(fan_btn, 94, 42);
    style_flat_button(fan_btn, 0x323233, LV_OPA_TRANSP, 10);
    lv_obj_add_event_cb(fan_btn, fan_clicked_cb, LV_EVENT_CLICKED, NULL);
    g_fan_label = make_label(fan_btn, "三级风力", 7, 10, 80, 22, 0xffffff);

    add_ac_illustration(ac_card, 25, 68);
    g_ac_temp_label = make_label(ac_card, "25°C", 360, 78, 75, 31, 0xffffff);

    lv_obj_t * ac_power_btn = make_icon_button(ac_card, 454, 69, 43, 43, 0xff9100,
                                               "\xEE\xA2\xAC", 25, 0xffffff, ac_power_clicked_cb);
    g_ac_power_icon = lv_obj_get_child(ac_power_btn, 0);

    lv_obj_t * curtain_card = lv_button_create(content);
    lv_obj_set_pos(curtain_card, 20, 365);
    lv_obj_set_size(curtain_card, 251, 135);
    style_flat_button(curtain_card, 0x303030, LV_OPA_COVER, 17);
    lv_obj_add_event_cb(curtain_card, card_clicked_cb, LV_EVENT_CLICKED, "窗帘详细控制");
    make_label(curtain_card, "窗帘", 32, 28, 70, 24, 0xffffff);

    lv_obj_t * curtain_power_btn = make_icon_button(curtain_card, 35, 65, 43, 43, 0xff9100,
                                                    "\xEE\xA2\xAC", 25, 0xffffff, curtain_power_clicked_cb);
    g_curtain_power_icon = lv_obj_get_child(curtain_power_btn, 0);
    add_curtain_illustration(curtain_card, 136, 31);

    lv_obj_t * light_card = lv_button_create(content);
    lv_obj_set_pos(light_card, 288, 365);
    lv_obj_set_size(light_card, 252, 135);
    style_flat_button(light_card, 0x303030, LV_OPA_COVER, 17);
    lv_obj_add_event_cb(light_card, card_clicked_cb, LV_EVENT_CLICKED, "客厅吊灯详细控制");
    make_label(light_card, "客厅吊灯", 28, 28, 110, 24, 0xffffff);

    lv_obj_t * light_power_btn = make_icon_button(light_card, 31, 65, 43, 43, 0x28c34a,
                                                  "\xEE\xA2\xAC", 25, 0xffffff, light_power_clicked_cb);
    g_light_power_icon = lv_obj_get_child(light_power_btn, 0);
    add_lamp_illustration(light_card, 143, 17);

    g_status_label = make_label(content, "自动模式", 24, 506, 180, 18, 0xffffff);
    lv_obj_set_style_text_opa(g_status_label, LV_OPA_70, 0);

    lv_obj_t * bottom_glass = lv_obj_create(shell);
    lv_obj_remove_flag(bottom_glass, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(bottom_glass, 76, 641);
    lv_obj_set_size(bottom_glass, 506, 16);
    lv_obj_set_style_radius(bottom_glass, 8, 0);
    lv_obj_set_style_border_width(bottom_glass, 0, 0);
    lv_obj_set_style_bg_color(bottom_glass, lv_color_hex(0xf4f4f4), 0);
    lv_obj_set_style_bg_opa(bottom_glass, LV_OPA_60, 0);

    update_mode_buttons();
    update_power_visuals();

    g_content_root = content;
    return screen;
}

lv_obj_t * web_f1249c8a_page_get_content_root(void)
{
    return g_content_root;
}
