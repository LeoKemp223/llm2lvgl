#include "stitch_smart_home_panel_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static void style_plain(lv_obj_t * obj)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void style_card(lv_obj_t * obj, lv_color_t bg, lv_coord_t radius)
{
    style_plain(obj);
    lv_obj_set_style_bg_color(obj, bg, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_shadow_width(obj, 14, 0);
    lv_obj_set_style_shadow_opa(obj, 10, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x191c1d), 0);
    lv_obj_set_style_shadow_offset_y(obj, 4, 0);
}

static lv_obj_t * make_label(lv_obj_t * parent, const char * text, int size, uint32_t color, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

static lv_obj_t * make_chip(lv_obj_t * parent, const char * text, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, uint32_t bg, uint32_t fg, lv_coord_t radius)
{
    lv_obj_t * chip = lv_obj_create(parent);
    style_plain(chip);
    lv_obj_set_style_bg_color(chip, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(chip, radius, 0);
    lv_obj_set_pos(chip, x, y);
    lv_obj_set_size(chip, w, h);

    lv_obj_t * label = lv_label_create(chip);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(11), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(fg), 0);
    lv_obj_center(label);
    return chip;
}

static lv_obj_t * make_icon_badge(lv_obj_t * parent, const char * symbol, lv_coord_t x, lv_coord_t y, lv_coord_t size, uint32_t bg, uint32_t fg)
{
    lv_obj_t * badge = lv_obj_create(parent);
    style_plain(badge);
    lv_obj_set_style_bg_color(badge, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(badge, 12, 0);
    lv_obj_set_pos(badge, x, y);
    lv_obj_set_size(badge, size, size);

    lv_obj_t * label = lv_label_create(badge);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_font(label, ui_font_get(18), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(fg), 0);
    lv_obj_center(label);
    return badge;
}

static lv_obj_t * make_round_button(lv_obj_t * parent, const char * text, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t * btn = lv_button_create(parent);
    style_plain(btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xedeeef), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 20, 0);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 40, 40);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(18), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0x191c1d), 0);
    lv_obj_center(label);
    return btn;
}

static lv_obj_t * make_action_button(lv_obj_t * parent, const char * text, lv_coord_t x, lv_coord_t y, lv_coord_t w, uint32_t bg, uint32_t fg)
{
    lv_obj_t * btn = lv_button_create(parent);
    style_plain(btn);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, 30);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(11), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(fg), 0);
    lv_obj_center(label);
    return btn;
}

static lv_obj_t * make_footer_item(lv_obj_t * parent, const char * icon, const char * text, lv_coord_t x, bool active)
{
    lv_obj_t * item = lv_obj_create(parent);
    style_plain(item);
    lv_obj_set_pos(item, x, 7);
    lv_obj_set_size(item, active ? 92 : 72, 42);
    lv_obj_set_style_radius(item, 12, 0);
    lv_obj_set_style_bg_opa(item, active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(item, lv_color_hex(active ? 0x0061ff : 0xf8f9fa), 0);

    lv_obj_t * icon_label = lv_label_create(item);
    lv_label_set_text(icon_label, icon);
    lv_obj_set_style_text_font(icon_label, ui_font_get(16), 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(active ? 0xffffff : 0x5e6263), 0);
    lv_obj_align(icon_label, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t * text_label = lv_label_create(item);
    lv_label_set_text(text_label, text);
    lv_obj_set_style_text_font(text_label, ui_font_get(10), 0);
    lv_obj_set_style_text_color(text_label, lv_color_hex(active ? 0xffffff : 0x5e6263), 0);
    lv_obj_align(text_label, LV_ALIGN_BOTTOM_MID, 0, -4);
    return item;
}

lv_obj_t * stitch_smart_home_panel_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    style_plain(screen);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xf8f9fa), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_size(screen, 480, 480);

    lv_obj_t * header = lv_obj_create(screen);
    style_plain(header);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xf8f9fa), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_size(header, 480, 56);
    lv_obj_set_pos(header, 0, 0);

    make_icon_badge(header, LV_SYMBOL_HOME, 16, 12, 30, 0xe6efff, 0x0061ff);
    make_label(header, "Good Morning", 16, 0x191c1d, 54, 15);
    make_label(header, "08:42 AM", 12, 0x191c1d, 360, 11);
    make_label(header, "72°F Sunny", 10, 0x5e6263, 357, 27);
    make_icon_badge(header, LV_SYMBOL_WIFI, 428, 12, 30, 0xe6efff, 0x0061ff);

    lv_obj_t * thermostat = lv_obj_create(screen);
    style_card(thermostat, lv_color_hex(0xffffff), 18);
    lv_obj_set_pos(thermostat, 16, 64);
    lv_obj_set_size(thermostat, 448, 88);

    make_label(thermostat, "THERMOSTAT", 10, 0x0061ff, 18, 16);
    make_label(thermostat, "72", 38, 0x191c1d, 18, 30);
    make_label(thermostat, "°F", 18, 0x424656, 78, 49);
    make_label(thermostat, "Heating to 74°F", 11, 0x5e6263, 18, 66);
    make_round_button(thermostat, LV_SYMBOL_PLUS, 388, 12);
    make_round_button(thermostat, LV_SYMBOL_MINUS, 388, 48);

    lv_obj_t * lights = lv_obj_create(screen);
    style_card(lights, lv_color_hex(0xffffff), 18);
    lv_obj_set_pos(lights, 16, 164);
    lv_obj_set_size(lights, 216, 108);

    make_icon_badge(lights, "L", 16, 14, 32, 0xe9f0ff, 0x0061ff);
    lv_obj_t * sw = lv_switch_create(lights);
    lv_obj_set_pos(sw, 164, 18);
    lv_obj_set_size(sw, 36, 20);
    lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xe1e3e4), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(sw, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x0061ff), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(sw, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(sw, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(sw, 0, LV_PART_KNOB);

    make_label(lights, "Main Lights", 14, 0x191c1d, 16, 60);
    make_label(lights, "80% Brightness", 11, 0x5e6263, 16, 78);

    lv_obj_t * slider = lv_slider_create(lights);
    lv_obj_set_pos(slider, 16, 94);
    lv_obj_set_size(slider, 184, 6);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 80, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xe1e3e4), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x0061ff), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);

    lv_obj_t * security = lv_obj_create(screen);
    style_card(security, lv_color_hex(0xffffff), 18);
    lv_obj_set_pos(security, 248, 164);
    lv_obj_set_size(security, 216, 108);

    make_icon_badge(security, "S", 16, 14, 32, 0xffefe8, 0x9d3000);
    make_label(security, "ARMED", 10, 0x9d3000, 162, 18);
    make_label(security, "Security", 14, 0x191c1d, 16, 60);
    make_label(security, "All sensors active", 11, 0x5e6263, 16, 78);
    make_action_button(security, "SETTINGS", 110, 72, 90, 0xe1e3e4, 0x191c1d);

    lv_obj_t * windows = lv_obj_create(screen);
    style_card(windows, lv_color_hex(0xffffff), 18);
    lv_obj_set_pos(windows, 16, 284);
    lv_obj_set_size(windows, 448, 60);

    make_icon_badge(windows, "W", 16, 14, 32, 0xe8efff, 0x435ba2);
    make_label(windows, "Living Windows", 14, 0x191c1d, 62, 12);
    make_label(windows, "Blinds 40% open", 11, 0x5e6263, 62, 30);
    make_action_button(windows, "OPEN", 318, 15, 54, 0xf3f4f5, 0x191c1d);
    make_action_button(windows, "CLOSE", 378, 15, 54, 0x0061ff, 0xffffff);

    make_label(screen, "QUICK SCENES", 10, 0x5e6263, 18, 358);
    make_chip(screen, LV_SYMBOL_HOME " Home", 16, 378, 96, 34, 0x0061ff, 0xffffff, 17);
    make_chip(screen, "Away", 122, 378, 72, 34, 0xe1e3e4, 0x191c1d, 17);
    make_chip(screen, "Sleep", 204, 378, 76, 34, 0xe1e3e4, 0x191c1d, 17);

    lv_obj_t * footer = lv_obj_create(screen);
    style_plain(footer);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0xf8f9fa), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(footer, 20, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_shadow_width(footer, 18, 0);
    lv_obj_set_style_shadow_opa(footer, 8, 0);
    lv_obj_set_style_shadow_color(footer, lv_color_hex(0x191c1d), 0);
    lv_obj_set_style_shadow_offset_y(footer, -2, 0);
    lv_obj_set_pos(footer, 0, 422);
    lv_obj_set_size(footer, 480, 58);

    make_footer_item(footer, LV_SYMBOL_HOME, "Home", 16, true);
    make_footer_item(footer, "C", "Climate", 122, false);
    make_footer_item(footer, "L", "Lights", 220, false);
    make_footer_item(footer, LV_SYMBOL_SETTINGS, "Settings", 318, false);

    g_content_root = screen;
    return screen;
}

lv_obj_t * stitch_smart_home_panel_page_get_content_root(void)
{
    return g_content_root;
}
