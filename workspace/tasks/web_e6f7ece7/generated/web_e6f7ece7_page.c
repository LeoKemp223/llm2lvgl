// Controls: 15 button(主页, 进入餐厅, 离开餐厅, 烛光晚餐, 休闲会客, 温馨就餐, 更多场景, 照明卡片, 冷暖新风卡片, 遮阳卡片, 家电卡片, 家具家私卡片, 上一首, 暂停, 下一首), 1 dropdown(餐厅, selected)
#include "web_e6f7ece7_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;
static lv_obj_t * g_feedback_label = NULL;
static lv_obj_t * g_more_overlay = NULL;
static lv_obj_t * g_pause_icon = NULL;
static lv_obj_t * g_song_label = NULL;
static bool g_music_playing = true;

static void set_text_style(lv_obj_t * obj, uint32_t color, int32_t size, lv_text_align_t align)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_font(obj, ui_font_get(size), 0);
    lv_obj_set_style_text_align(obj, align, 0);
}

static void base_panel_style(lv_obj_t * obj, uint32_t bg, lv_opa_t opa, int32_t radius)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

static void button_card_style(lv_obj_t * btn, uint32_t bg, lv_opa_t opa, int32_t radius)
{
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, opa, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3a3b43), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_90, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x444650), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_90, LV_PART_MAIN | LV_STATE_CHECKED);
}

static void command_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    const char * text = (const char *)lv_event_get_user_data(e);
    if(text && g_feedback_label) {
        lv_label_set_text(g_feedback_label, text);
    }

    lv_obj_t * target = lv_event_get_target(e);
    if(target) {
        if(lv_obj_has_state(target, LV_STATE_CHECKED)) lv_obj_remove_state(target, LV_STATE_CHECKED);
        else lv_obj_add_state(target, LV_STATE_CHECKED);
    }
}

static void home_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(g_feedback_label) lv_label_set_text(g_feedback_label, "已切换到主页");
}

static void more_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(g_more_overlay) lv_obj_remove_flag(g_more_overlay, LV_OBJ_FLAG_HIDDEN);
    if(g_feedback_label) lv_label_set_text(g_feedback_label, "更多场景已打开");
}

static void close_more_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if(g_more_overlay) lv_obj_add_flag(g_more_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void media_prev_next_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    const char * text = (const char *)lv_event_get_user_data(e);
    if(g_song_label && text) lv_label_set_text(g_song_label, text);
    if(g_feedback_label && text) lv_label_set_text(g_feedback_label, text);
}

static void pause_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    g_music_playing = !g_music_playing;
    if(g_pause_icon) {
        lv_label_set_text(g_pause_icon, g_music_playing ? "\xEE\x80\xB4" : "\xEE\x80\xB7");
    }
    if(g_feedback_label) {
        lv_label_set_text(g_feedback_label, g_music_playing ? "音乐正在播放" : "音乐已暂停");
    }
}

static void room_dropdown_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;

    lv_obj_t * dd = lv_event_get_target(e);
    char buf[32];
    lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
    if(g_feedback_label) {
        char msg[64];
        lv_snprintf(msg, sizeof(msg), "当前房间：%s", buf);
        lv_label_set_text(g_feedback_label, msg);
    }
}

static lv_obj_t * create_scene_button(lv_obj_t * parent, int32_t x, int32_t y, uint32_t accent, const char * icon_text, const char * title, const char * feedback)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 234, 168);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    button_card_style(btn, 0x303138, LV_OPA_90, 16);
    lv_obj_add_event_cb(btn, command_event_cb, LV_EVENT_CLICKED, (void *)feedback);

    lv_obj_t * dot = lv_obj_create(btn);
    base_panel_style(dot, accent, LV_OPA_COVER, 24);
    lv_obj_set_size(dot, 48, 48);
    lv_obj_set_pos(dot, 16, 58);

    lv_obj_t * icon = lv_label_create(dot);
    lv_label_set_text(icon, icon_text);
    lv_obj_set_style_text_font(icon, ui_icon_font_get(25), 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xffffff), 0);
    lv_obj_center(icon);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, title);
    set_text_style(label, 0xf7f7f9, 27, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(label, 103, 68);

    return btn;
}

static lv_obj_t * create_status_card(lv_obj_t * parent, int32_t x, int32_t y, uint32_t bg, uint32_t glow, const char * title, const char * subtitle, const char * value, const char * footer, const char * feedback)
{
    lv_obj_t * card = lv_button_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, 347, 281);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CHECKABLE);
    button_card_style(card, bg, 232, 21);
    lv_obj_set_style_bg_grad_color(card, lv_color_hex(0x17191f), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_add_event_cb(card, command_event_cb, LV_EVENT_CLICKED, (void *)feedback);

    lv_obj_t * title_label = lv_label_create(card);
    lv_label_set_text(title_label, title);
    set_text_style(title_label, 0xffffff, 28, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(title_label, 24, 21);

    lv_obj_t * sub_label = lv_label_create(card);
    lv_label_set_text(sub_label, subtitle);
    set_text_style(sub_label, 0xb6b6bd, 24, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(sub_label, 24, 74);

    lv_obj_t * value_label = lv_label_create(card);
    lv_label_set_text(value_label, value);
    set_text_style(value_label, 0xffffff, 39, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(value_label, 24, 114);

    if(footer && footer[0] != '\0') {
        lv_obj_t * footer_label = lv_label_create(card);
        lv_label_set_text(footer_label, footer);
        set_text_style(footer_label, 0xc5c5cb, 23, LV_TEXT_ALIGN_LEFT);
        lv_obj_set_pos(footer_label, 24, 226);
    }

    lv_obj_t * disk = lv_obj_create(card);
    base_panel_style(disk, glow, 120, 70);
    lv_obj_set_size(disk, 128, 58);
    lv_obj_set_pos(disk, 179, 66);
    lv_obj_set_style_shadow_width(disk, 38, 0);
    lv_obj_set_style_shadow_color(disk, lv_color_hex(glow), 0);
    lv_obj_set_style_shadow_opa(disk, 100, 0);

    lv_obj_t * plate = lv_obj_create(card);
    base_panel_style(plate, 0xffffff, 38, 8);
    lv_obj_set_size(plate, 138, 40);
    lv_obj_set_pos(plate, 205, 42);
    lv_obj_set_style_transform_rotation(plate, 260, 0);

    return card;
}

static lv_obj_t * create_media_button(lv_obj_t * parent, int32_t x, int32_t y, int32_t size, const char * icon_text)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, size, size);
    button_card_style(btn, 0x242236, LV_OPA_TRANSP, size / 2);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);

    lv_obj_t * icon = lv_label_create(btn);
    lv_label_set_text(icon, icon_text);
    lv_obj_set_style_text_font(icon, ui_icon_font_get(size - 18), 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xf4f4f7), 0);
    lv_obj_center(icon);
    return btn;
}

static void create_background(lv_obj_t * screen)
{
    lv_obj_t * bg = lv_obj_create(screen);
    lv_obj_set_size(bg, 1704, 774);
    lv_obj_set_pos(bg, 0, 0);
    base_panel_style(bg, 0x07080a, LV_OPA_COVER, 0);

    lv_obj_t * shade = lv_obj_create(bg);
    lv_obj_set_size(shade, 1704, 774);
    lv_obj_set_pos(shade, 0, 0);
    base_panel_style(shade, 0x101419, LV_OPA_80, 0);
    lv_obj_set_style_bg_grad_color(shade, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_grad_dir(shade, LV_GRAD_DIR_HOR, 0);

    lv_obj_t * window = lv_obj_create(bg);
    lv_obj_set_size(window, 345, 345);
    lv_obj_set_pos(window, 986, 8);
    base_panel_style(window, 0x111a23, 70, 173);
    lv_obj_set_style_border_width(window, 8, 0);
    lv_obj_set_style_border_color(window, lv_color_hex(0xe4e7ee), 0);
    lv_obj_set_style_border_opa(window, 130, 0);

    lv_obj_t * chair = lv_obj_create(bg);
    lv_obj_set_size(chair, 235, 250);
    lv_obj_set_pos(chair, 1207, 45);
    base_panel_style(chair, 0x101a2b, 210, 118);
    lv_obj_set_style_bg_grad_color(chair, lv_color_hex(0x1a2943), 0);
    lv_obj_set_style_bg_grad_dir(chair, LV_GRAD_DIR_VER, 0);

    for(int i = 0; i < 9; i++) {
        lv_obj_t * line = lv_obj_create(bg);
        base_panel_style(line, 0x26303a, 40, 0);
        lv_obj_set_size(line, 3, 774);
        lv_obj_set_pos(line, 930 + i * 58, 0);
    }
}

static void create_more_overlay(lv_obj_t * screen)
{
    g_more_overlay = lv_obj_create(screen);
    lv_obj_set_size(g_more_overlay, 520, 330);
    lv_obj_center(g_more_overlay);
    base_panel_style(g_more_overlay, 0x22242b, LV_OPA_90, 24);
    lv_obj_set_style_border_width(g_more_overlay, 1, 0);
    lv_obj_set_style_border_color(g_more_overlay, lv_color_hex(0x575a64), 0);
    lv_obj_add_flag(g_more_overlay, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t * title = lv_label_create(g_more_overlay);
    lv_label_set_text(title, "更多餐厅场景");
    set_text_style(title, 0xffffff, 30, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(title, 34, 30);

    const char * items[] = {"节能模式", "明亮清洁", "夜间微光", "聚会模式"};
    for(int i = 0; i < 4; i++) {
        lv_obj_t * btn = lv_button_create(g_more_overlay);
        lv_obj_set_size(btn, 190, 74);
        lv_obj_set_pos(btn, 34 + (i % 2) * 220, 92 + (i / 2) * 94);
        button_card_style(btn, 0x333540, LV_OPA_COVER, 14);
        lv_obj_add_event_cb(btn, command_event_cb, LV_EVENT_CLICKED, (void *)items[i]);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        set_text_style(label, 0xffffff, 24, LV_TEXT_ALIGN_CENTER);
        lv_obj_center(label);
    }

    lv_obj_t * close = lv_button_create(g_more_overlay);
    lv_obj_set_size(close, 54, 54);
    lv_obj_set_pos(close, 436, 24);
    button_card_style(close, 0x393b45, LV_OPA_COVER, 27);
    lv_obj_add_event_cb(close, close_more_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * icon = lv_label_create(close);
    lv_label_set_text(icon, "\xEE\x85\x8C");
    lv_obj_set_style_text_font(icon, ui_icon_font_get(26), 0);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xffffff), 0);
    lv_obj_center(icon);
}

lv_obj_t * web_e6f7ece7_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 1704, 774);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1c1d1f), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    create_background(screen);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_set_size(content, 1704, 774);
    lv_obj_set_pos(content, 0, 0);
    base_panel_style(content, 0x000000, LV_OPA_TRANSP, 0);

    lv_obj_t * home = lv_button_create(content);
    lv_obj_set_pos(home, 17, 23);
    lv_obj_set_size(home, 102, 101);
    button_card_style(home, 0x24262b, LV_OPA_90, 24);
    lv_obj_add_event_cb(home, home_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * home_icon = lv_label_create(home);
    lv_label_set_text(home_icon, "\xEE\xA2\x8A");
    lv_obj_set_style_text_font(home_icon, ui_icon_font_get(55), 0);
    lv_obj_set_style_text_color(home_icon, lv_color_hex(0xf1f1f4), 0);
    lv_obj_center(home_icon);

    lv_obj_t * room = lv_dropdown_create(content);
    lv_obj_set_pos(room, 149, 20);
    lv_obj_set_size(room, 306, 105);
    lv_dropdown_set_options(room, "餐厅\n客厅\n卧室\n厨房\n书房");
    lv_dropdown_set_selected(room, 0);
    lv_obj_set_style_bg_color(room, lv_color_hex(0x202126), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(room, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(room, 31, LV_PART_MAIN);
    lv_obj_set_style_border_width(room, 4, LV_PART_MAIN);
    lv_obj_set_style_border_color(room, lv_color_hex(0xf4f4f7), LV_PART_MAIN);
    lv_obj_set_style_border_opa(room, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_font(room, ui_font_get(32), LV_PART_MAIN);
    lv_obj_set_style_text_color(room, lv_color_hex(0xf0f0f3), LV_PART_MAIN);
    lv_obj_set_style_pad_left(room, 102, LV_PART_MAIN);
    lv_obj_set_style_pad_top(room, 27, LV_PART_MAIN);
    lv_obj_add_event_cb(room, room_dropdown_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t * dining_icon = lv_label_create(content);
    lv_label_set_text(dining_icon, "\xEE\x95\x87");
    lv_obj_set_style_text_font(dining_icon, ui_icon_font_get(45), 0);
    lv_obj_set_style_text_color(dining_icon, lv_color_hex(0xffffff), 0);
    lv_obj_set_pos(dining_icon, 178, 51);

    lv_obj_t * scene_panel = lv_obj_create(content);
    lv_obj_set_pos(scene_panel, 17, 170);
    lv_obj_set_size(scene_panel, 530, 592);
    base_panel_style(scene_panel, 0x24262b, LV_OPA_90, 24);

    create_scene_button(scene_panel, 20, 22, 0x3b9ee8, "\xEE\xA2\x8A", "进入餐厅", "已执行：进入餐厅");
    create_scene_button(scene_panel, 276, 22, 0x5cc8c2, "\xEE\x97\x84", "离开餐厅", "已执行：离开餐厅");
    create_scene_button(scene_panel, 20, 213, 0xf36f6e, "\xEE\x83\xB0", "烛光晚餐", "已执行：烛光晚餐");
    create_scene_button(scene_panel, 276, 213, 0xdc5d8c, "\xEE\x95\x87", "休闲会客", "已执行：休闲会客");
    create_scene_button(scene_panel, 20, 404, 0xffbd68, "\xEE\x95\x87", "温馨就餐", "已执行：温馨就餐");

    lv_obj_t * more = create_scene_button(scene_panel, 276, 404, 0x494b54, "\xEE\xA2\x96", "更多场景", "更多场景");
    lv_obj_add_event_cb(more, more_event_cb, LV_EVENT_CLICKED, NULL);

    create_status_card(content, 577, 170, 0x352522, 0xd48b22, "照明", "灯光", "未开启", "0盏灯开", "进入照明详情");
    create_status_card(content, 954, 170, 0x1d3336, 0x23aaa6, "冷暖新风", "空气质量", "优", "温度 24°C | 湿度 40%", "进入冷暖新风详情");

    lv_obj_t * music = lv_button_create(content);
    lv_obj_set_pos(music, 1330, 170);
    lv_obj_set_size(music, 348, 281);
    lv_obj_add_flag(music, LV_OBJ_FLAG_CHECKABLE);
    button_card_style(music, 0x252137, 238, 21);
    lv_obj_set_style_bg_grad_color(music, lv_color_hex(0x191827), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(music, LV_GRAD_DIR_HOR, LV_PART_MAIN);
    lv_obj_add_event_cb(music, command_event_cb, LV_EVENT_CLICKED, (void *)"进入音乐详情");

    lv_obj_t * music_title = lv_label_create(music);
    lv_label_set_text(music_title, "音乐");
    set_text_style(music_title, 0xffffff, 28, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(music_title, 24, 21);

    lv_obj_t * artist = lv_label_create(music);
    lv_label_set_text(artist, "Delacey");
    set_text_style(artist, 0xbdbbc7, 24, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(artist, 24, 74);

    g_song_label = lv_label_create(music);
    lv_label_set_text(g_song_label, "Dream it Possible");
    set_text_style(g_song_label, 0xffffff, 37, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(g_song_label, 24, 116);

    lv_obj_t * note_disk = lv_obj_create(music);
    base_panel_style(note_disk, 0x7249d9, 135, 55);
    lv_obj_set_size(note_disk, 112, 58);
    lv_obj_set_pos(note_disk, 190, 74);
    lv_obj_set_style_shadow_width(note_disk, 40, 0);
    lv_obj_set_style_shadow_color(note_disk, lv_color_hex(0x7447de), 0);
    lv_obj_set_style_shadow_opa(note_disk, 115, 0);

    lv_obj_t * note = lv_label_create(music);
    lv_label_set_text(note, "\xEE\x81\x90");
    lv_obj_set_style_text_font(note, ui_icon_font_get(82), 0);
    lv_obj_set_style_text_color(note, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_opa(note, 70, 0);
    lv_obj_set_pos(note, 221, 22);

    lv_obj_t * prev = create_media_button(music, 24, 184, 62, "\xEE\x97\x84");
    lv_obj_add_event_cb(prev, media_prev_next_event_cb, LV_EVENT_CLICKED, (void *)"上一首：Memory Lane");

    lv_obj_t * pause = create_media_button(music, 132, 173, 78, "\xEE\x80\xB4");
    lv_obj_set_style_border_width(pause, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(pause, lv_color_hex(0xf0f0f3), LV_PART_MAIN);
    lv_obj_set_style_border_opa(pause, LV_OPA_90, LV_PART_MAIN);
    lv_obj_add_event_cb(pause, pause_event_cb, LV_EVENT_CLICKED, NULL);
    g_pause_icon = lv_obj_get_child(pause, 0);

    lv_obj_t * next = create_media_button(music, 250, 184, 62, "\xEE\x97\x88");
    lv_obj_add_event_cb(next, media_prev_next_event_cb, LV_EVENT_CLICKED, (void *)"下一首：Dream it Possible");

    create_status_card(content, 577, 480, 0x352127, 0xc25e2d, "遮阳", "紫外线指数", "较低", "", "进入遮阳详情");
    create_status_card(content, 954, 480, 0x202842, 0x376fd7, "家电", "扫地机器人", "已清扫20%", "", "进入扫地机器人详情");
    create_status_card(content, 1330, 480, 0x283326, 0x74a64b, "家具家私", "按摩椅", "正在运行", "", "进入按摩椅详情");

    lv_obj_t * robot_icon = lv_label_create(content);
    lv_label_set_text(robot_icon, "\xEE\x90\xA5");
    lv_obj_set_style_text_font(robot_icon, ui_icon_font_get(34), 0);
    lv_obj_set_style_text_color(robot_icon, lv_color_hex(0xd4d4d8), 0);
    lv_obj_set_pos(robot_icon, 1241, 695);

    lv_obj_t * chair_icon = lv_label_create(content);
    lv_label_set_text(chair_icon, "\xEE\x9F\xBD");
    lv_obj_set_style_text_font(chair_icon, ui_icon_font_get(35), 0);
    lv_obj_set_style_text_color(chair_icon, lv_color_hex(0xd4d4d8), 0);
    lv_obj_set_pos(chair_icon, 1617, 694);

    g_feedback_label = lv_label_create(content);
    lv_label_set_text(g_feedback_label, "当前房间：餐厅");
    set_text_style(g_feedback_label, 0xbfc0c6, 18, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_pos(g_feedback_label, 35, 744);

    create_more_overlay(screen);

    g_content_root = content;
    return screen;
}

lv_obj_t * web_e6f7ece7_page_get_content_root(void)
{
    return g_content_root;
}
