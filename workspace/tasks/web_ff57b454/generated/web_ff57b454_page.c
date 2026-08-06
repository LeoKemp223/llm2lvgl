// Controls: 15 buttons(主页, 进入餐厅, 离开餐厅, 烛光晚餐, 休闲会客, 温馨就餐, 更多场景, 照明, 冷暖新风, 音乐卡片, 上一首, 暂停, 下一首, 遮阳, 家电, 家具家私), 1 dropdown(餐厅)
#include "web_ff57b454_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;
static lv_obj_t * g_status_label = NULL;
static lv_obj_t * g_play_icon = NULL;
static bool g_music_playing = true;

typedef struct {
    lv_obj_t * label;
    const char * active_text;
} button_action_ctx_t;

static button_action_ctx_t g_home_ctx;
static button_action_ctx_t g_scene_ctx[6];
static button_action_ctx_t g_card_ctx[6];
static button_action_ctx_t g_music_prev_ctx;
static button_action_ctx_t g_music_next_ctx;

static void style_plain_container(lv_obj_t * obj)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void button_action_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    button_action_ctx_t * ctx = (button_action_ctx_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_transform_scale(lv_event_get_target_obj(e), 245, 0);
    } else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_transform_scale(lv_event_get_target_obj(e), 256, 0);
    } else if(code == LV_EVENT_CLICKED && ctx != NULL && g_status_label != NULL) {
        lv_label_set_text(g_status_label, ctx->active_text);
    }
}

static void music_toggle_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target_obj(e);

    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_transform_scale(target, 245, 0);
    } else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_transform_scale(target, 256, 0);
    } else if(code == LV_EVENT_CLICKED) {
        g_music_playing = !g_music_playing;
        if(g_play_icon != NULL) {
            lv_label_set_text(g_play_icon, g_music_playing ? "\xEE\x80\xB4" : "\xEE\x80\xB7");
        }
        if(g_status_label != NULL) {
            lv_label_set_text(g_status_label, g_music_playing ? "音乐播放中" : "音乐已暂停");
        }
    }
}

static void room_dropdown_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || g_status_label == NULL) return;

    lv_obj_t * dropdown = lv_event_get_target_obj(e);
    char buf[32];
    lv_dropdown_get_selected_str(dropdown, buf, sizeof(buf));
    lv_label_set_text_fmt(g_status_label, "当前房间：%s", buf);
}

static lv_obj_t * make_soft_glow(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color, lv_opa_t opa)
{
    lv_obj_t * glow = lv_obj_create(parent);
    style_plain_container(glow);
    lv_obj_remove_flag(glow, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(glow, x, y);
    lv_obj_set_size(glow, w, h);
    lv_obj_set_style_radius(glow, h / 2, 0);
    lv_obj_set_style_bg_color(glow, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(glow, opa, 0);
    return glow;
}

static lv_obj_t * make_label(lv_obj_t * parent, const char * text, int32_t x, int32_t y, int32_t font_size, uint32_t color, lv_opa_t opa)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_font_get(font_size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_opa(label, opa, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, 0);
    return label;
}

static lv_obj_t * make_icon_label(lv_obj_t * parent, const char * text, int32_t size, uint32_t color)
{
    lv_obj_t * icon = lv_label_create(parent);
    lv_label_set_text(icon, text);
    lv_obj_set_style_text_font(icon, ui_icon_font_get(size), 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, 0);
    return icon;
}

static lv_obj_t * make_round_icon(lv_obj_t * parent, int32_t x, int32_t y, int32_t d, uint32_t bg, const char * icon_text, int32_t icon_size)
{
    lv_obj_t * circle = lv_obj_create(parent);
    style_plain_container(circle);
    lv_obj_remove_flag(circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(circle, x, y);
    lv_obj_set_size(circle, d, d);
    lv_obj_set_style_radius(circle, d / 2, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);

    lv_obj_t * icon = make_icon_label(circle, icon_text, icon_size, 0xffffff);
    lv_obj_center(icon);
    return circle;
}

static lv_obj_t * make_scene_button(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                                    const char * label_text, uint32_t icon_bg, const char * icon_text,
                                    int32_t icon_size, button_action_ctx_t * ctx, const char * active_text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 20, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x303137), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, 232, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3d4048), LV_PART_MAIN | LV_STATE_PRESSED);

    make_round_icon(btn, 16, 56, 58, icon_bg, icon_text, icon_size);

    lv_obj_t * title = lv_label_create(btn);
    lv_label_set_text(title, label_text);
    lv_obj_set_pos(title, 104, 64);
    lv_obj_set_style_text_font(title, ui_font_get(28), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xf5f7fb), 0);
    lv_obj_set_style_bg_opa(title, LV_OPA_TRANSP, 0);

    ctx->label = title;
    ctx->active_text = active_text;
    lv_obj_add_event_cb(btn, button_action_event_cb, LV_EVENT_ALL, ctx);
    return btn;
}

static void make_plate_art(lv_obj_t * parent, int32_t x, int32_t y, uint32_t base, uint32_t top, lv_opa_t opa)
{
    lv_obj_t * base_disc = lv_obj_create(parent);
    style_plain_container(base_disc);
    lv_obj_remove_flag(base_disc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(base_disc, x, y + 44);
    lv_obj_set_size(base_disc, 150, 62);
    lv_obj_set_style_radius(base_disc, 31, 0);
    lv_obj_set_style_bg_color(base_disc, lv_color_hex(base), 0);
    lv_obj_set_style_bg_opa(base_disc, opa, 0);
    lv_obj_set_style_shadow_width(base_disc, 18, 0);
    lv_obj_set_style_shadow_color(base_disc, lv_color_hex(base), 0);
    lv_obj_set_style_shadow_opa(base_disc, 70, 0);

    lv_obj_t * top_disc = lv_obj_create(parent);
    style_plain_container(top_disc);
    lv_obj_remove_flag(top_disc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(top_disc, x + 20, y + 16);
    lv_obj_set_size(top_disc, 160, 58);
    lv_obj_set_style_radius(top_disc, 29, 0);
    lv_obj_set_style_bg_color(top_disc, lv_color_hex(top), 0);
    lv_obj_set_style_bg_opa(top_disc, 82, 0);
    lv_obj_set_style_transform_rotation(top_disc, -40, 0);

    lv_obj_t * slab = lv_obj_create(parent);
    style_plain_container(slab);
    lv_obj_remove_flag(slab, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(slab, x + 68, y);
    lv_obj_set_size(slab, 150, 44);
    lv_obj_set_style_radius(slab, 9, 0);
    lv_obj_set_style_bg_color(slab, lv_color_hex(top), 0);
    lv_obj_set_style_bg_opa(slab, 70, 0);
    lv_obj_set_style_transform_rotation(slab, 310, 0);
}

static lv_obj_t * make_device_card(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                                   uint32_t bg, uint32_t grad, const char * title,
                                   const char * sub, const char * main_text, const char * foot,
                                   uint32_t art_base, uint32_t art_top,
                                   button_action_ctx_t * ctx, const char * active_text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 26, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(btn, lv_color_hex(grad), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(btn, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, 221, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3a3c46), LV_PART_MAIN | LV_STATE_PRESSED);

    make_soft_glow(btn, w - 178, 38, 128, 74, art_base, 42);
    make_plate_art(btn, w - 170, 42, art_base, art_top, 120);

    make_label(btn, title, 24, 22, 29, 0xffffff, LV_OPA_COVER);

    if(sub != NULL && sub[0] != '\0') {
        make_label(btn, sub, 24, 74, 25, 0xc6c9d1, 184);
    }

    if(main_text != NULL && main_text[0] != '\0') {
        make_label(btn, main_text, 24, 112, 42, 0xffffff, LV_OPA_COVER);
    }

    if(foot != NULL && foot[0] != '\0') {
        make_label(btn, foot, 24, h - 52, 24, 0xd7d9df, 194);
    }

    ctx->active_text = active_text;
    ctx->label = NULL;
    lv_obj_add_event_cb(btn, button_action_event_cb, LV_EVENT_ALL, ctx);
    return btn;
}

static void add_music_controls(lv_obj_t * card)
{
    lv_obj_t * prev = lv_button_create(card);
    lv_obj_set_pos(prev, 28, 194);
    lv_obj_set_size(prev, 68, 58);
    lv_obj_set_style_radius(prev, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(prev, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(prev, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(prev, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(prev, lv_color_hex(0x3c3553), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_t * prev_icon = make_icon_label(prev, "\xEE\x80\xB7", 40, 0xf7f4fb);
    lv_obj_set_style_transform_rotation(prev_icon, 1800, 0);
    lv_obj_center(prev_icon);
    g_music_prev_ctx.active_text = "上一首";
    g_music_prev_ctx.label = NULL;
    lv_obj_add_event_cb(prev, button_action_event_cb, LV_EVENT_ALL, &g_music_prev_ctx);

    lv_obj_t * play = lv_button_create(card);
    lv_obj_set_pos(play, 135, 181);
    lv_obj_set_size(play, 80, 80);
    lv_obj_set_style_radius(play, 40, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(play, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_color(play, lv_color_hex(0xd8d5df), LV_PART_MAIN);
    lv_obj_set_style_border_width(play, 3, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(play, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(play, lv_color_hex(0x3c3553), LV_PART_MAIN | LV_STATE_PRESSED);
    g_play_icon = make_icon_label(play, "\xEE\x80\xB4", 42, 0xf7f4fb);
    lv_obj_center(g_play_icon);
    lv_obj_add_event_cb(play, music_toggle_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * next = lv_button_create(card);
    lv_obj_set_pos(next, 253, 194);
    lv_obj_set_size(next, 68, 58);
    lv_obj_set_style_radius(next, 16, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(next, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(next, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(next, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(next, lv_color_hex(0x3c3553), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_t * next_icon = make_icon_label(next, "\xEE\x80\xB7", 40, 0xf7f4fb);
    lv_obj_center(next_icon);
    g_music_next_ctx.active_text = "下一首";
    g_music_next_ctx.label = NULL;
    lv_obj_add_event_cb(next, button_action_event_cb, LV_EVENT_ALL, &g_music_next_ctx);
}

lv_obj_t * web_ff57b454_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 1704, 774);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x050607), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    g_content_root = screen;

    lv_obj_t * bg = lv_obj_create(screen);
    style_plain_container(bg);
    lv_obj_set_size(bg, 1704, 774);
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_set_style_bg_color(bg, lv_color_hex(0x050607), 0);
    lv_obj_set_style_bg_grad_color(bg, lv_color_hex(0x121923), 0);
    lv_obj_set_style_bg_grad_dir(bg, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);

    make_soft_glow(bg, 930, -16, 500, 420, 0x2a4460, 48);
    make_soft_glow(bg, 1195, 42, 260, 250, 0x142a4d, 85);
    make_soft_glow(bg, 1520, 142, 210, 190, 0x0b1220, 100);

    lv_obj_t * ring = lv_obj_create(bg);
    style_plain_container(ring);
    lv_obj_remove_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(ring, 985, 5);
    lv_obj_set_size(ring, 312, 312);
    lv_obj_set_style_radius(ring, 156, 0);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ring, 7, 0);
    lv_obj_set_style_border_color(ring, lv_color_hex(0xdde3ec), 0);
    lv_obj_set_style_border_opa(ring, 165, 0);

    lv_obj_t * chair = lv_obj_create(bg);
    style_plain_container(chair);
    lv_obj_remove_flag(chair, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(chair, 1210, 46);
    lv_obj_set_size(chair, 250, 250);
    lv_obj_set_style_radius(chair, 125, 0);
    lv_obj_set_style_bg_color(chair, lv_color_hex(0x17233a), 0);
    lv_obj_set_style_bg_grad_color(chair, lv_color_hex(0x07101c), 0);
    lv_obj_set_style_bg_grad_dir(chair, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_bg_opa(chair, 210, 0);

    for(int i = 0; i < 11; i++) {
        lv_obj_t * line = lv_obj_create(bg);
        style_plain_container(line);
        lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(line, 1085 + i * 38, 0);
        lv_obj_set_size(line, 3, 360);
        lv_obj_set_style_bg_color(line, lv_color_hex(0x243140), 0);
        lv_obj_set_style_bg_opa(line, 45, 0);
    }

    lv_obj_t * top_home = lv_button_create(screen);
    lv_obj_set_pos(top_home, 17, 24);
    lv_obj_set_size(top_home, 101, 101);
    lv_obj_set_style_radius(top_home, 25, LV_PART_MAIN);
    lv_obj_set_style_bg_color(top_home, lv_color_hex(0x222429), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top_home, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_home, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(top_home, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(top_home, lv_color_hex(0x33363d), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_t * home_icon = make_icon_label(top_home, "\xEE\xA2\x8A", 57, 0xf1f2f6);
    lv_obj_center(home_icon);
    g_home_ctx.active_text = "返回主页";
    g_home_ctx.label = NULL;
    lv_obj_add_event_cb(top_home, button_action_event_cb, LV_EVENT_ALL, &g_home_ctx);

    lv_obj_t * room_dd = lv_dropdown_create(screen);
    lv_obj_set_pos(room_dd, 149, 20);
    lv_obj_set_size(room_dd, 306, 106);
    lv_dropdown_set_options(room_dd, "餐厅\n客厅\n卧室\n厨房\n书房");
    lv_dropdown_set_selected(room_dd, 0);
    lv_obj_set_style_radius(room_dd, 32, LV_PART_MAIN);
    lv_obj_set_style_bg_color(room_dd, lv_color_hex(0x202126), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(room_dd, 238, LV_PART_MAIN);
    lv_obj_set_style_border_color(room_dd, lv_color_hex(0xf2f3f7), LV_PART_MAIN);
    lv_obj_set_style_border_width(room_dd, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_left(room_dd, 103, LV_PART_MAIN);
    lv_obj_set_style_pad_right(room_dd, 42, LV_PART_MAIN);
    lv_obj_set_style_text_font(room_dd, ui_font_get(34), LV_PART_MAIN);
    lv_obj_set_style_text_color(room_dd, lv_color_hex(0xf3f4f8), LV_PART_MAIN);
    lv_obj_set_style_bg_color(room_dd, lv_color_hex(0x2f3138), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_font(room_dd, ui_font_get(28), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(room_dd, lv_color_hex(0x2a2c32), LV_PART_ITEMS);
    lv_obj_set_style_text_color(room_dd, lv_color_hex(0xffffff), LV_PART_ITEMS);
    lv_obj_add_event_cb(room_dd, room_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * dining_icon = make_icon_label(screen, "\xEE\x95\x87", 42, 0xf5f6fa);
    lv_obj_set_pos(dining_icon, 177, 51);

    lv_obj_t * left_panel = lv_obj_create(screen);
    style_plain_container(left_panel);
    lv_obj_set_pos(left_panel, 16, 171);
    lv_obj_set_size(left_panel, 532, 591);
    lv_obj_set_style_radius(left_panel, 25, 0);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(0x202126), 0);
    lv_obj_set_style_bg_opa(left_panel, 235, 0);

    make_scene_button(left_panel, 22, 21, 232, 168, "进入餐厅", 0x45a5df, "\xEE\xA2\x8D", 33, &g_scene_ctx[0], "已执行：进入餐厅");
    make_scene_button(left_panel, 276, 21, 234, 168, "离开餐厅", 0x5ccbc4, "\xEE\xA2\x8D", 33, &g_scene_ctx[1], "已执行：离开餐厅");
    make_scene_button(left_panel, 22, 212, 232, 168, "烛光晚餐", 0xee706b, "\xEE\x83\xB0", 32, &g_scene_ctx[2], "已执行：烛光晚餐");
    make_scene_button(left_panel, 276, 212, 234, 168, "休闲会客", 0xdc5a8b, "\xEE\x95\x87", 31, &g_scene_ctx[3], "已执行：休闲会客");
    make_scene_button(left_panel, 22, 402, 232, 168, "温馨就餐", 0xffbd6a, "\xEE\x95\x87", 32, &g_scene_ctx[4], "已执行：温馨就餐");
    make_scene_button(left_panel, 276, 402, 234, 168, "更多场景", 0x585b63, "\xEE\xA2\x96", 34, &g_scene_ctx[5], "更多场景菜单已打开");

    lv_obj_t * light_card = make_device_card(screen, 578, 170, 346, 280, 0x302322, 0x463026,
                                            "照明", "灯光", "未开启", "0盏灯开",
                                            0xe89a1d, 0x94816a, &g_card_ctx[0], "打开照明详情");
    (void)light_card;

    lv_obj_t * air_card = make_device_card(screen, 954, 170, 346, 280, 0x172a30, 0x1b3840,
                                          "冷暖新风", "空气质量", "优", "温度 24°C | 湿度 40%",
                                          0x1ab7ad, 0x6fa6a1, &g_card_ctx[1], "打开冷暖新风详情");
    (void)air_card;

    lv_obj_t * music_card = make_device_card(screen, 1329, 170, 347, 280, 0x211d34, 0x231c3d,
                                            "音乐", "Delacey", "Dream it Possible", "",
                                            0x7144dd, 0x62517c, &g_card_ctx[2], "打开音乐详情");
    add_music_controls(music_card);

    lv_obj_t * shade_card = make_device_card(screen, 578, 481, 346, 280, 0x2d1d22, 0x3a2324,
                                            "遮阳", "紫外线指数", "较低", "",
                                            0xc96539, 0x7f625b, &g_card_ctx[3], "打开遮阳详情");
    (void)shade_card;

    lv_obj_t * appliance_card = make_device_card(screen, 954, 481, 346, 280, 0x1b2234, 0x1f2945,
                                                "家电", "扫地机器人", "已清扫20%", "",
                                                0x2d6bc9, 0x65728f, &g_card_ctx[4], "打开家电详情");
    lv_obj_t * robot_icon = make_icon_label(appliance_card, "\xEE\x90\xA5", 35, 0xbec2cb);
    lv_obj_set_pos(robot_icon, 287, 214);

    lv_obj_t * furniture_card = make_device_card(screen, 1329, 481, 347, 280, 0x20291c, 0x263220,
                                                "家具家私", "按摩椅", "正在运行", "",
                                                0x79a94d, 0x738469, &g_card_ctx[5], "打开家具设备详情");
    lv_obj_t * chair_icon = make_icon_label(furniture_card, "\xEE\x95\x87", 35, 0xd7dad6);
    lv_obj_set_pos(chair_icon, 287, 215);

    g_status_label = lv_label_create(screen);
    lv_label_set_text(g_status_label, "当前房间：餐厅");
    lv_obj_set_pos(g_status_label, 149, 135);
    lv_obj_set_style_text_font(g_status_label, ui_font_get(18), 0);
    lv_obj_set_style_text_color(g_status_label, lv_color_hex(0xbec2ca), 0);
    lv_obj_set_style_text_opa(g_status_label, 150, 0);
    lv_obj_set_style_bg_opa(g_status_label, LV_OPA_TRANSP, 0);

    return screen;
}

lv_obj_t * web_ff57b454_page_get_content_root(void)
{
    return g_content_root;
}
