#include "web_02df6568_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static void style_plain(lv_obj_t * obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static lv_obj_t * create_text(lv_obj_t * parent, const char * text, int32_t x, int32_t y, int32_t size, uint32_t color)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    lv_obj_set_pos(label, x, y);
    return label;
}

static void create_clock_icon(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * clock = lv_obj_create(parent);
    style_plain(clock);
    lv_obj_set_size(clock, 44, 44);
    lv_obj_set_pos(clock, x, y);
    lv_obj_set_style_radius(clock, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(clock, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clock, 3, 0);
    lv_obj_set_style_border_color(clock, lv_color_hex(0xf2f2f2), 0);

    lv_obj_t * hand_v = lv_obj_create(clock);
    style_plain(hand_v);
    lv_obj_set_size(hand_v, 3, 13);
    lv_obj_set_pos(hand_v, 20, 8);
    lv_obj_set_style_radius(hand_v, 2, 0);
    lv_obj_set_style_bg_color(hand_v, lv_color_hex(0xf2f2f2), 0);
    lv_obj_set_style_bg_opa(hand_v, LV_OPA_COVER, 0);

    lv_obj_t * hand_h = lv_obj_create(clock);
    style_plain(hand_h);
    lv_obj_set_size(hand_h, 11, 3);
    lv_obj_set_pos(hand_h, 21, 21);
    lv_obj_set_style_radius(hand_h, 2, 0);
    lv_obj_set_style_bg_color(hand_h, lv_color_hex(0xf2f2f2), 0);
    lv_obj_set_style_bg_opa(hand_h, LV_OPA_COVER, 0);
}

static void create_tomato_icon(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * tomato = lv_obj_create(parent);
    style_plain(tomato);
    lv_obj_set_size(tomato, 44, 44);
    lv_obj_set_pos(tomato, x, y);
    lv_obj_set_style_radius(tomato, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tomato, lv_color_hex(0xff6b2c), 0);
    lv_obj_set_style_bg_opa(tomato, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tomato, 2, 0);
    lv_obj_set_style_border_color(tomato, lv_color_hex(0xff8a55), 0);

    lv_obj_t * core = lv_obj_create(tomato);
    style_plain(core);
    lv_obj_set_size(core, 8, 8);
    lv_obj_center(core);
    lv_obj_set_style_radius(core, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(core, lv_color_hex(0xffb083), 0);
    lv_obj_set_style_bg_opa(core, 160, 0);

    static lv_point_precise_t pts1[] = {{5, 22}, {39, 22}};
    static lv_point_precise_t pts2[] = {{22, 5}, {22, 39}};
    static lv_point_precise_t pts3[] = {{9, 9}, {35, 35}};
    static lv_point_precise_t pts4[] = {{35, 9}, {9, 35}};

    lv_obj_t * l1 = lv_line_create(tomato);
    lv_line_set_points(l1, pts1, 2);
    lv_obj_set_style_line_width(l1, 2, 0);
    lv_obj_set_style_line_color(l1, lv_color_hex(0xff9c70), 0);
    lv_obj_set_style_line_opa(l1, 150, 0);

    lv_obj_t * l2 = lv_line_create(tomato);
    lv_line_set_points(l2, pts2, 2);
    lv_obj_set_style_line_width(l2, 2, 0);
    lv_obj_set_style_line_color(l2, lv_color_hex(0xff9c70), 0);
    lv_obj_set_style_line_opa(l2, 150, 0);

    lv_obj_t * l3 = lv_line_create(tomato);
    lv_line_set_points(l3, pts3, 2);
    lv_obj_set_style_line_width(l3, 2, 0);
    lv_obj_set_style_line_color(l3, lv_color_hex(0xff9c70), 0);
    lv_obj_set_style_line_opa(l3, 150, 0);

    lv_obj_t * l4 = lv_line_create(tomato);
    lv_line_set_points(l4, pts4, 2);
    lv_obj_set_style_line_width(l4, 2, 0);
    lv_obj_set_style_line_color(l4, lv_color_hex(0xff9c70), 0);
    lv_obj_set_style_line_opa(l4, 150, 0);
}

static void create_right_illustration(lv_obj_t * parent)
{
    lv_obj_t * fade = lv_obj_create(parent);
    style_plain(fade);
    lv_obj_set_size(fade, 430, 404);
    lv_obj_set_pos(fade, 1675, 0);
    lv_obj_set_style_bg_color(fade, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_color(fade, lv_color_hex(0xf0f0ef), 0);
    lv_obj_set_style_bg_grad_dir(fade, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(fade, LV_OPA_COVER, 0);

    lv_obj_t * clock_shadow = lv_obj_create(fade);
    style_plain(clock_shadow);
    lv_obj_set_size(clock_shadow, 258, 258);
    lv_obj_set_pos(clock_shadow, 138, 81);
    lv_obj_set_style_radius(clock_shadow, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(clock_shadow, lv_color_hex(0x113d72), 0);
    lv_obj_set_style_bg_opa(clock_shadow, 75, 0);

    lv_obj_t * clock_outer = lv_obj_create(fade);
    style_plain(clock_outer);
    lv_obj_set_size(clock_outer, 250, 250);
    lv_obj_set_pos(clock_outer, 124, 67);
    lv_obj_set_style_radius(clock_outer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(clock_outer, lv_color_hex(0x1559a7), 0);
    lv_obj_set_style_bg_opa(clock_outer, LV_OPA_COVER, 0);

    lv_obj_t * clock_face = lv_obj_create(clock_outer);
    style_plain(clock_face);
    lv_obj_set_size(clock_face, 214, 214);
    lv_obj_center(clock_face);
    lv_obj_set_style_radius(clock_face, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(clock_face, lv_color_hex(0xd6d7d6), 0);
    lv_obj_set_style_bg_opa(clock_face, LV_OPA_COVER, 0);

    lv_obj_t * center = lv_obj_create(clock_face);
    style_plain(center);
    lv_obj_set_size(center, 20, 20);
    lv_obj_set_pos(center, 97, 98);
    lv_obj_set_style_radius(center, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(center, lv_color_hex(0x164f9c), 0);
    lv_obj_set_style_bg_opa(center, LV_OPA_COVER, 0);

    lv_obj_t * hand1 = lv_obj_create(clock_face);
    style_plain(hand1);
    lv_obj_set_size(hand1, 8, 77);
    lv_obj_set_pos(hand1, 103, 33);
    lv_obj_set_style_radius(hand1, 4, 0);
    lv_obj_set_style_bg_color(hand1, lv_color_hex(0xc67622), 0);
    lv_obj_set_style_bg_opa(hand1, LV_OPA_COVER, 0);

    static lv_point_precise_t hand_pts[] = {{106, 108}, {73, 172}};
    lv_obj_t * hand2 = lv_line_create(clock_face);
    lv_line_set_points(hand2, hand_pts, 2);
    lv_obj_set_style_line_width(hand2, 5, 0);
    lv_obj_set_style_line_color(hand2, lv_color_hex(0x164f9c), 0);
    lv_obj_set_style_line_rounded(hand2, true, 0);

    lv_obj_t * tick;
    tick = lv_obj_create(clock_face);
    style_plain(tick);
    lv_obj_set_size(tick, 8, 18);
    lv_obj_set_pos(tick, 103, 10);
    lv_obj_set_style_radius(tick, 4, 0);
    lv_obj_set_style_bg_color(tick, lv_color_hex(0xbc741d), 0);
    lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);

    tick = lv_obj_create(clock_face);
    style_plain(tick);
    lv_obj_set_size(tick, 8, 18);
    lv_obj_set_pos(tick, 103, 186);
    lv_obj_set_style_radius(tick, 4, 0);
    lv_obj_set_style_bg_color(tick, lv_color_hex(0xbc741d), 0);
    lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);

    tick = lv_obj_create(clock_face);
    style_plain(tick);
    lv_obj_set_size(tick, 18, 8);
    lv_obj_set_pos(tick, 12, 104);
    lv_obj_set_style_radius(tick, 4, 0);
    lv_obj_set_style_bg_color(tick, lv_color_hex(0xbc741d), 0);
    lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);

    tick = lv_obj_create(clock_face);
    style_plain(tick);
    lv_obj_set_size(tick, 18, 8);
    lv_obj_set_pos(tick, 184, 104);
    lv_obj_set_style_radius(tick, 4, 0);
    lv_obj_set_style_bg_color(tick, lv_color_hex(0xbc741d), 0);
    lv_obj_set_style_bg_opa(tick, LV_OPA_COVER, 0);

    lv_obj_t * digital = lv_obj_create(fade);
    style_plain(digital);
    lv_obj_set_size(digital, 162, 78);
    lv_obj_set_pos(digital, 292, 40);
    lv_obj_set_style_radius(digital, 10, 0);
    lv_obj_set_style_bg_color(digital, lv_color_hex(0xf46b20), 0);
    lv_obj_set_style_bg_opa(digital, LV_OPA_COVER, 0);

    lv_obj_t * digital_inner = lv_obj_create(digital);
    style_plain(digital_inner);
    lv_obj_set_size(digital_inner, 124, 47);
    lv_obj_set_pos(digital_inner, 20, 15);
    lv_obj_set_style_radius(digital_inner, 4, 0);
    lv_obj_set_style_bg_color(digital_inner, lv_color_hex(0x2e72b8), 0);
    lv_obj_set_style_bg_opa(digital_inner, LV_OPA_COVER, 0);

    lv_obj_t * time = lv_label_create(digital_inner);
    lv_label_set_text(time, "00:30");
    lv_obj_set_style_text_font(time, ui_font_get(28), 0);
    lv_obj_set_style_text_color(time, lv_color_hex(0xddeefd), 0);
    lv_obj_center(time);

    lv_obj_t * calendar = lv_obj_create(fade);
    style_plain(calendar);
    lv_obj_set_size(calendar, 138, 96);
    lv_obj_set_pos(calendar, 318, 286);
    lv_obj_set_style_radius(calendar, 10, 0);
    lv_obj_set_style_bg_color(calendar, lv_color_hex(0xf5f6f6), 0);
    lv_obj_set_style_bg_opa(calendar, LV_OPA_COVER, 0);

    lv_obj_t * cal_top = lv_obj_create(calendar);
    style_plain(cal_top);
    lv_obj_set_size(cal_top, 138, 22);
    lv_obj_set_pos(cal_top, 0, 0);
    lv_obj_set_style_radius(cal_top, 8, 0);
    lv_obj_set_style_bg_color(cal_top, lv_color_hex(0x1b63ad), 0);
    lv_obj_set_style_bg_opa(cal_top, LV_OPA_COVER, 0);

    for(int r = 0; r < 3; r++) {
        for(int c = 0; c < 5; c++) {
            lv_obj_t * sq = lv_obj_create(calendar);
            style_plain(sq);
            lv_obj_set_size(sq, 16, 16);
            lv_obj_set_pos(sq, 18 + c * 22, 34 + r * 20);
            lv_obj_set_style_radius(sq, 2, 0);
            lv_obj_set_style_bg_color(sq, lv_color_hex(0xe07a1d), 0);
            lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, 0);
        }
    }

    lv_obj_t * person_head = lv_obj_create(fade);
    style_plain(person_head);
    lv_obj_set_size(person_head, 25, 25);
    lv_obj_set_pos(person_head, 374, 122);
    lv_obj_set_style_radius(person_head, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(person_head, lv_color_hex(0x1f5d9c), 0);
    lv_obj_set_style_bg_opa(person_head, LV_OPA_COVER, 0);

    lv_obj_t * person_body = lv_obj_create(fade);
    style_plain(person_body);
    lv_obj_set_size(person_body, 31, 91);
    lv_obj_set_pos(person_body, 364, 149);
    lv_obj_set_style_radius(person_body, 16, 0);
    lv_obj_set_style_bg_color(person_body, lv_color_hex(0xc86924), 0);
    lv_obj_set_style_bg_opa(person_body, LV_OPA_COVER, 0);

    lv_obj_t * leaf1 = lv_obj_create(fade);
    style_plain(leaf1);
    lv_obj_set_size(leaf1, 34, 146);
    lv_obj_set_pos(leaf1, 42, 192);
    lv_obj_set_style_radius(leaf1, 18, 0);
    lv_obj_set_style_bg_color(leaf1, lv_color_hex(0xd8781d), 0);
    lv_obj_set_style_bg_opa(leaf1, 180, 0);

    lv_obj_t * small_clock = lv_obj_create(fade);
    style_plain(small_clock);
    lv_obj_set_size(small_clock, 76, 76);
    lv_obj_set_pos(small_clock, 64, 75);
    lv_obj_set_style_radius(small_clock, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(small_clock, lv_color_hex(0xd4d4d1), 0);
    lv_obj_set_style_bg_opa(small_clock, 190, 0);
    lv_obj_set_style_border_width(small_clock, 8, 0);
    lv_obj_set_style_border_color(small_clock, lv_color_hex(0xc46b1f), 0);
}

lv_obj_t * web_02df6568_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    style_plain(screen);
    lv_obj_set_size(screen, 2105, 404);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t * content = lv_obj_create(screen);
    style_plain(content);
    lv_obj_set_size(content, 2105, 404);
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);

    create_right_illustration(content);

    create_text(content, "←", 16, 15, 45, 0xf5f5f5);
    create_tomato_icon(content, 89, 15);
    create_text(content, "番茄时钟设置", 153, 16, 34, 0xf1f1f1);

    create_clock_icon(content, 387, 185);
    create_text(content, "学习时长:", 442, 187, 34, 0xf3f3f3);

    create_text(content, "30min", 674, 119, 31, 0xf0f0f0);
    create_text(content, "40min", 662, 190, 38, 0xf5f5f5);
    create_text(content, "50min", 679, 263, 31, 0xf0f0f0);

    create_clock_icon(content, 856, 185);
    create_text(content, "休息时长:", 911, 187, 34, 0xf3f3f3);

    create_text(content, "10min", 1140, 119, 31, 0xf0f0f0);
    create_text(content, "15min", 1129, 190, 38, 0xf5f5f5);
    create_text(content, "20min", 1146, 263, 31, 0xf0f0f0);

    lv_obj_t * start_btn = lv_button_create(content);
    lv_obj_remove_flag(start_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(start_btn, 256, 98);
    lv_obj_set_pos(start_btn, 1346, 155);
    lv_obj_set_style_radius(start_btn, 49, LV_PART_MAIN);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x1e91df), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(start_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(start_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(start_btn, 18, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(start_btn, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(start_btn, lv_color_hex(0x07101b), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(start_btn, 185, LV_PART_MAIN);

    lv_obj_t * btn_label = lv_label_create(start_btn);
    lv_label_set_text(btn_label, "开始学习");
    lv_obj_set_style_text_font(btn_label, ui_font_get(36), 0);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(btn_label);

    g_content_root = content;
    return screen;
}

lv_obj_t * web_02df6568_page_get_content_root(void)
{
    return g_content_root;
}
