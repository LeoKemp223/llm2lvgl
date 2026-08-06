// Controls: 1 button(返回), 6 button(radio time options: 学习时长 30/40/50 分钟, 休息时长 10/15/20 分钟), 1 button(开始学习)
#include "web_bd944f3c_page.h"

#include "ui_font.h"

#include <stdint.h>

static lv_obj_t * g_content_root = NULL;

static lv_obj_t * g_study_buttons[3];
static lv_obj_t * g_study_labels[3];
static lv_obj_t * g_break_buttons[3];
static lv_obj_t * g_break_labels[3];
static lv_obj_t * g_timer_label = NULL;
static int g_study_selected = 1;
static int g_break_selected = 1;

static void style_plain_container(lv_obj_t * obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static lv_obj_t * create_text(lv_obj_t * parent, const char * text, int32_t x, int32_t y, int32_t size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static lv_obj_t * create_icon_label(lv_obj_t * parent, const char * icon, int32_t x, int32_t y, int32_t size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, icon);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_icon_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static void update_option_group(lv_obj_t ** labels, lv_obj_t ** buttons, int selected)
{
    for(int i = 0; i < 3; i++) {
        bool is_selected = (i == selected);
        lv_obj_set_style_text_font(labels[i], ui_font_get(is_selected ? 38 : 32), 0);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(is_selected ? 0xffffff : 0xe6e6e6), 0);
        lv_obj_set_style_bg_opa(buttons[i], is_selected ? 18 : LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(buttons[i], lv_color_hex(0xffffff), 0);
        if(is_selected) {
            lv_obj_add_state(buttons[i], LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(buttons[i], LV_STATE_CHECKED);
        }
    }
}

static void update_selections(void)
{
    update_option_group(g_study_labels, g_study_buttons, g_study_selected);
    update_option_group(g_break_labels, g_break_buttons, g_break_selected);

    if(g_timer_label != NULL) {
        const char * times[] = { "00:30", "00:40", "00:50" };
        lv_label_set_text(g_timer_label, times[g_study_selected]);
    }
}

static void study_option_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        g_study_selected = (int)(intptr_t)lv_event_get_user_data(e);
        update_selections();
    }
}

static void break_option_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        g_break_selected = (int)(intptr_t)lv_event_get_user_data(e);
        update_selections();
    }
}

static void back_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t * btn = lv_event_get_target_obj(e);
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
}

static void start_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t * btn = lv_event_get_target_obj(e);
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
}

static lv_obj_t * create_option_button(lv_obj_t * parent, int32_t x, int32_t y, const char * text, lv_event_cb_t cb, int index)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 170, 64);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(btn, 35, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff), LV_STATE_PRESSED);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, (void *)(intptr_t)index);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_center(label);
    lv_obj_set_style_text_font(label, ui_font_get(32), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xe6e6e6), 0);
    return btn;
}

static void create_tomato_mark(lv_obj_t * parent)
{
    lv_obj_t * tomato = lv_obj_create(parent);
    style_plain_container(tomato);
    lv_obj_set_pos(tomato, 88, 15);
    lv_obj_set_size(tomato, 45, 45);
    lv_obj_set_style_radius(tomato, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tomato, lv_color_hex(0xff6d32), 0);
    lv_obj_set_style_bg_opa(tomato, LV_OPA_COVER, 0);

    lv_obj_t * center = lv_obj_create(tomato);
    style_plain_container(center);
    lv_obj_set_size(center, 8, 8);
    lv_obj_center(center);
    lv_obj_set_style_radius(center, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(center, lv_color_hex(0xff9b6b), 0);
    lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);

    for(int i = 0; i < 8; i++) {
        lv_obj_t * seed = lv_obj_create(tomato);
        style_plain_container(seed);
        lv_obj_set_size(seed, (i % 2) ? 3 : 18, (i % 2) ? 18 : 3);
        lv_obj_set_style_radius(seed, 2, 0);
        lv_obj_set_style_bg_color(seed, lv_color_hex(0xff9b6b), 0);
        lv_obj_set_style_bg_opa(seed, 180, 0);
        if(i == 0) lv_obj_set_pos(seed, 22, 4);
        if(i == 1) lv_obj_set_pos(seed, 22, 24);
        if(i == 2) lv_obj_set_pos(seed, 4, 22);
        if(i == 3) lv_obj_set_pos(seed, 24, 22);
        if(i == 4) lv_obj_set_pos(seed, 8, 10);
        if(i == 5) lv_obj_set_pos(seed, 30, 10);
        if(i == 6) lv_obj_set_pos(seed, 8, 32);
        if(i == 7) lv_obj_set_pos(seed, 30, 32);
    }
}

static void create_clock_illustration(lv_obj_t * parent)
{
    lv_obj_t * panel = lv_obj_create(parent);
    style_plain_container(panel);
    lv_obj_set_pos(panel, 1680, 0);
    lv_obj_set_size(panel, 425, 404);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x171717), 0);
    lv_obj_set_style_bg_grad_color(panel, lv_color_hex(0xd2d2d0), 0);
    lv_obj_set_style_bg_grad_dir(panel, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);

    lv_obj_t * shadow = lv_obj_create(panel);
    style_plain_container(shadow);
    lv_obj_set_pos(shadow, 132, 91);
    lv_obj_set_size(shadow, 238, 232);
    lv_obj_set_style_radius(shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(0x6f7c87), 0);
    lv_obj_set_style_bg_opa(shadow, 145, 0);

    lv_obj_t * clock_ring = lv_obj_create(panel);
    style_plain_container(clock_ring);
    lv_obj_set_pos(clock_ring, 120, 67);
    lv_obj_set_size(clock_ring, 250, 250);
    lv_obj_set_style_radius(clock_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(clock_ring, lv_color_hex(0x1e64ad), 0);
    lv_obj_set_style_bg_opa(clock_ring, LV_OPA_COVER, 0);

    lv_obj_t * face = lv_obj_create(clock_ring);
    style_plain_container(face);
    lv_obj_set_size(face, 214, 214);
    lv_obj_center(face);
    lv_obj_set_style_radius(face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(face, lv_color_hex(0xd8d9d7), 0);
    lv_obj_set_style_bg_opa(face, LV_OPA_COVER, 0);

    lv_obj_t * center = lv_obj_create(face);
    style_plain_container(center);
    lv_obj_set_size(center, 19, 19);
    lv_obj_set_pos(center, 99, 116);
    lv_obj_set_style_radius(center, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(center, lv_color_hex(0x175ba8), 0);
    lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);

    lv_obj_t * minute = lv_obj_create(face);
    style_plain_container(minute);
    lv_obj_set_size(minute, 8, 92);
    lv_obj_set_pos(minute, 103, 32);
    lv_obj_set_style_radius(minute, 5, 0);
    lv_obj_set_style_bg_color(minute, lv_color_hex(0xb46a18), 0);
    lv_obj_set_style_bg_opa(minute, LV_OPA_COVER, 0);

    lv_obj_t * hand = lv_obj_create(face);
    style_plain_container(hand);
    lv_obj_set_size(hand, 6, 82);
    lv_obj_set_pos(hand, 86, 119);
    lv_obj_set_style_radius(hand, 4, 0);
    lv_obj_set_style_bg_color(hand, lv_color_hex(0x1557a3), 0);
    lv_obj_set_style_bg_opa(hand, LV_OPA_COVER, 0);
    lv_obj_set_style_transform_rotation(hand, 250, 0);

    const int marks[4][4] = {
        {103, 10, 8, 18},
        {103, 187, 8, 18},
        {17, 105, 18, 8},
        {188, 105, 18, 8}
    };
    for(int i = 0; i < 4; i++) {
        lv_obj_t * mark = lv_obj_create(face);
        style_plain_container(mark);
        lv_obj_set_pos(mark, marks[i][0], marks[i][1]);
        lv_obj_set_size(mark, marks[i][2], marks[i][3]);
        lv_obj_set_style_radius(mark, 5, 0);
        lv_obj_set_style_bg_color(mark, lv_color_hex(0xb46a18), 0);
        lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
    }

    lv_obj_t * bell_l = lv_obj_create(panel);
    style_plain_container(bell_l);
    lv_obj_set_pos(bell_l, 60, 76);
    lv_obj_set_size(bell_l, 77, 77);
    lv_obj_set_style_radius(bell_l, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bell_l, lv_color_hex(0xc7741f), 0);
    lv_obj_set_style_bg_opa(bell_l, LV_OPA_COVER, 0);

    lv_obj_t * bell_l_inner = lv_obj_create(bell_l);
    style_plain_container(bell_l_inner);
    lv_obj_set_size(bell_l_inner, 61, 61);
    lv_obj_center(bell_l_inner);
    lv_obj_set_style_radius(bell_l_inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bell_l_inner, lv_color_hex(0xc9cbc8), 0);
    lv_obj_set_style_bg_opa(bell_l_inner, LV_OPA_COVER, 0);

    lv_obj_t * left_leg = lv_obj_create(panel);
    style_plain_container(left_leg);
    lv_obj_set_pos(left_leg, 37, 192);
    lv_obj_set_size(left_leg, 34, 146);
    lv_obj_set_style_radius(left_leg, 17, 0);
    lv_obj_set_style_bg_color(left_leg, lv_color_hex(0xb66a19), 0);
    lv_obj_set_style_bg_opa(left_leg, LV_OPA_COVER, 0);

    lv_obj_t * right_leg = lv_obj_create(panel);
    style_plain_container(right_leg);
    lv_obj_set_pos(right_leg, 360, 150);
    lv_obj_set_size(right_leg, 31, 90);
    lv_obj_set_style_radius(right_leg, 16, 0);
    lv_obj_set_style_bg_color(right_leg, lv_color_hex(0xd36f22), 0);
    lv_obj_set_style_bg_opa(right_leg, LV_OPA_COVER, 0);

    lv_obj_t * small_dot = lv_obj_create(panel);
    style_plain_container(small_dot);
    lv_obj_set_pos(small_dot, 372, 122);
    lv_obj_set_size(small_dot, 26, 26);
    lv_obj_set_style_radius(small_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(small_dot, lv_color_hex(0x2363a6), 0);
    lv_obj_set_style_bg_opa(small_dot, LV_OPA_COVER, 0);

    lv_obj_t * timer_card = lv_obj_create(panel);
    style_plain_container(timer_card);
    lv_obj_set_pos(timer_card, 288, 40);
    lv_obj_set_size(timer_card, 160, 78);
    lv_obj_set_style_radius(timer_card, 10, 0);
    lv_obj_set_style_bg_color(timer_card, lv_color_hex(0xff7423), 0);
    lv_obj_set_style_bg_opa(timer_card, LV_OPA_COVER, 0);

    lv_obj_t * timer_blue = lv_obj_create(timer_card);
    style_plain_container(timer_blue);
    lv_obj_set_pos(timer_blue, 18, 15);
    lv_obj_set_size(timer_blue, 130, 47);
    lv_obj_set_style_radius(timer_blue, 4, 0);
    lv_obj_set_style_bg_color(timer_blue, lv_color_hex(0x2267ad), 0);
    lv_obj_set_style_bg_opa(timer_blue, LV_OPA_COVER, 0);

    g_timer_label = create_text(timer_blue, "00:40", 25, 7, 30, 0xf4f6fb);

    lv_obj_t * calendar = lv_obj_create(panel);
    style_plain_container(calendar);
    lv_obj_set_pos(calendar, 313, 286);
    lv_obj_set_size(calendar, 142, 96);
    lv_obj_set_style_radius(calendar, 7, 0);
    lv_obj_set_style_bg_color(calendar, lv_color_hex(0xf7f7f5), 0);
    lv_obj_set_style_bg_opa(calendar, LV_OPA_COVER, 0);

    lv_obj_t * calendar_top = lv_obj_create(calendar);
    style_plain_container(calendar_top);
    lv_obj_set_pos(calendar_top, 0, 0);
    lv_obj_set_size(calendar_top, 142, 23);
    lv_obj_set_style_radius(calendar_top, 7, 0);
    lv_obj_set_style_bg_color(calendar_top, lv_color_hex(0x2267ad), 0);
    lv_obj_set_style_bg_opa(calendar_top, LV_OPA_COVER, 0);

    for(int r = 0; r < 3; r++) {
        for(int c = 0; c < 5; c++) {
            lv_obj_t * sq = lv_obj_create(calendar);
            style_plain_container(sq);
            lv_obj_set_pos(sq, 18 + c * 22, 34 + r * 20);
            lv_obj_set_size(sq, 16, 16);
            lv_obj_set_style_radius(sq, 3, 0);
            lv_obj_set_style_bg_color(sq, lv_color_hex(0xe97d17), 0);
            lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, 0);
        }
    }
}

lv_obj_t * web_bd944f3c_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 2105, 404);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * content = lv_obj_create(screen);
    g_content_root = content;
    style_plain_container(content);
    lv_obj_set_size(content, 2105, 404);
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);

    lv_obj_t * back_btn = lv_button_create(content);
    lv_obj_set_pos(back_btn, 0, 18);
    lv_obj_set_size(back_btn, 75, 60);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(back_btn, 35, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xffffff), LV_STATE_PRESSED);
    lv_obj_set_style_border_width(back_btn, 0, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_set_style_radius(back_btn, 8, 0);
    lv_obj_add_event_cb(back_btn, back_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * back_icon = lv_label_create(back_btn);
    lv_label_set_text(back_icon, "\xEE\x97\x84");
    lv_obj_set_style_text_font(back_icon, ui_icon_font_get(42), 0);
    lv_obj_set_style_text_color(back_icon, lv_color_hex(0xf2f2f2), 0);
    lv_obj_center(back_icon);

    create_tomato_mark(content);
    create_text(content, "番茄时钟设置", 154, 24, 36, 0xf5f5f5);

    create_icon_label(content, "\xEE\x86\x92", 386, 185, 46, 0xf0f0f0);
    create_text(content, "学习时长:", 443, 194, 37, 0xf0f0f0);

    g_study_buttons[0] = create_option_button(content, 640, 112, "30min", study_option_event_cb, 0);
    g_study_buttons[1] = create_option_button(content, 640, 184, "40min", study_option_event_cb, 1);
    g_study_buttons[2] = create_option_button(content, 640, 256, "50min", study_option_event_cb, 2);
    for(int i = 0; i < 3; i++) {
        g_study_labels[i] = lv_obj_get_child(g_study_buttons[i], 0);
    }

    create_icon_label(content, "\xEE\x86\x92", 856, 185, 46, 0xf0f0f0);
    create_text(content, "休息时长:", 912, 194, 37, 0xf0f0f0);

    g_break_buttons[0] = create_option_button(content, 1105, 112, "10min", break_option_event_cb, 0);
    g_break_buttons[1] = create_option_button(content, 1105, 184, "15min", break_option_event_cb, 1);
    g_break_buttons[2] = create_option_button(content, 1105, 256, "20min", break_option_event_cb, 2);
    for(int i = 0; i < 3; i++) {
        g_break_labels[i] = lv_obj_get_child(g_break_buttons[i], 0);
    }

    lv_obj_t * start_btn = lv_button_create(content);
    lv_obj_set_pos(start_btn, 1346, 155);
    lv_obj_set_size(start_btn, 257, 98);
    lv_obj_set_style_radius(start_btn, 49, 0);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x2896dc), 0);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x177fc3), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(start_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(start_btn, 0, 0);
    lv_obj_set_style_shadow_width(start_btn, 14, 0);
    lv_obj_set_style_shadow_color(start_btn, lv_color_hex(0x071016), 0);
    lv_obj_set_style_shadow_opa(start_btn, 135, 0);
    lv_obj_set_style_shadow_offset_y(start_btn, 12, 0);
    lv_obj_add_event_cb(start_btn, start_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "开始学习");
    lv_obj_set_style_text_font(start_label, ui_font_get(38), 0);
    lv_obj_set_style_text_color(start_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(start_label);

    create_clock_illustration(content);
    update_selections();

    return screen;
}

lv_obj_t * web_bd944f3c_page_get_content_root(void)
{
    return g_content_root;
}
