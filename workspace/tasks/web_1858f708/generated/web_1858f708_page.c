#include "web_1858f708_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static lv_obj_t * create_text(lv_obj_t * parent, const char * text, int32_t x, int32_t y,
                              int32_t font_size, uint32_t color, lv_text_align_t align)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, ui_font_get(font_size), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_set_style_text_align(label, align, 0);
    return label;
}

static void create_logo_icon(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    lv_obj_t * left = lv_obj_create(parent);
    lv_obj_remove_flag(left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(left, x, y);
    lv_obj_set_size(left, w / 2 + 2, h);
    lv_obj_set_style_radius(left, 2, 0);
    lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left, 2, 0);
    lv_obj_set_style_border_color(left, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_pad_all(left, 0, 0);

    lv_obj_t * right = lv_obj_create(parent);
    lv_obj_remove_flag(right, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(right, x + w / 2 - 2, y);
    lv_obj_set_size(right, w / 2 + 2, h);
    lv_obj_set_style_radius(right, 2, 0);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 2, 0);
    lv_obj_set_style_border_color(right, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_pad_all(right, 0, 0);

    lv_obj_t * spine = lv_obj_create(parent);
    lv_obj_remove_flag(spine, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(spine, x + w / 2 - 1, y + 2);
    lv_obj_set_size(spine, 2, h - 4);
    lv_obj_set_style_bg_color(spine, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_bg_opa(spine, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(spine, 0, 0);
    lv_obj_set_style_radius(spine, 0, 0);
}

static void create_company_icon(lv_obj_t * parent, int32_t x, int32_t y)
{
    lv_obj_t * body = lv_obj_create(parent);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(body, x, y);
    lv_obj_set_size(body, 18, 24);
    lv_obj_set_style_radius(body, 3, 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(body, 2, 0);
    lv_obj_set_style_border_color(body, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_pad_all(body, 0, 0);

    lv_obj_t * side = lv_obj_create(parent);
    lv_obj_remove_flag(side, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(side, x + 14, y + 8);
    lv_obj_set_size(side, 12, 16);
    lv_obj_set_style_radius(side, 2, 0);
    lv_obj_set_style_bg_opa(side, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(side, 2, 0);
    lv_obj_set_style_border_color(side, lv_color_hex(0x2563eb), 0);
    lv_obj_set_style_pad_all(side, 0, 0);

    for(int i = 0; i < 3; i++) {
        lv_obj_t * line = lv_obj_create(parent);
        lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(line, x + 5, y + 5 + i * 6);
        lv_obj_set_size(line, 8, 2);
        lv_obj_set_style_bg_color(line, lv_color_hex(0x2563eb), 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(line, 0, 0);
        lv_obj_set_style_radius(line, 1, 0);
    }
}

static void create_nav_text(lv_obj_t * parent, const char * text, int32_t x)
{
    lv_obj_t * label = create_text(parent, text, x, 29, 15, 0x020817, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_size(label, 70, 22);
}

static void create_active_nav(lv_obj_t * parent)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, 252, 13);
    lv_obj_set_size(btn, 49, 40);
    lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2563eb), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x2563eb), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    lv_obj_t * label = lv_label_create(btn);
    lv_label_set_text(label, "首页");
    lv_obj_set_style_text_font(label, ui_font_get(15), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_center(label);
}

static void create_top_icon_label(lv_obj_t * parent, const char * text, int32_t x)
{
    lv_obj_t * label = create_text(parent, text, x, 27, 22, 0x020817, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_size(label, 28, 28);
}

lv_obj_t * web_1858f708_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, 1280, 800);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, 0);
    lv_obj_set_size(content, 1280, 800);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    g_content_root = content;

    lv_obj_t * header = lv_obj_create(content);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_size(header, 1280, 88);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);

    lv_obj_t * divider = lv_obj_create(content);
    lv_obj_remove_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(divider, 0, 87);
    lv_obj_set_size(divider, 1280, 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0xe2e8f0), 0);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);
    lv_obj_set_style_radius(divider, 0, 0);

    create_logo_icon(header, 15, 23, 22, 25);
    create_text(header, "面包刷题", 47, 19, 23, 0x020817, LV_TEXT_ALIGN_LEFT);

    create_active_nav(header);
    create_nav_text(header, "题库", 315);
    create_nav_text(header, "企业库", 368);
    create_nav_text(header, "练习记录", 434);
    create_nav_text(header, "收藏", 517);
    create_nav_text(header, "刷题技巧", 570);
    create_nav_text(header, "简历模板", 648);
    create_nav_text(header, "名企校招", 729);
    create_nav_text(header, "职经分享", 807);
    create_nav_text(header, "更多资料", 886);

    create_top_icon_label(header, "☼", 1074);
    create_top_icon_label(header, "⊙", 1119);
    create_top_icon_label(header, "♙", 1166);
    create_text(header, "小智学长", 1201, 29, 15, 0x020817, LV_TEXT_ALIGN_LEFT);

    lv_obj_t * hero = lv_obj_create(content);
    lv_obj_remove_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(hero, 0, 88);
    lv_obj_set_size(hero, 1280, 712);
    lv_obj_set_style_bg_color(hero, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hero, 0, 0);
    lv_obj_set_style_radius(hero, 0, 0);
    lv_obj_set_style_pad_all(hero, 0, 0);

    lv_obj_t * title = create_text(hero, "面包刷题", 0, 272, 58, 0x4f46e5, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_width(title, 1280);

    lv_obj_t * subtitle = create_text(hero, "专注于嵌入式领域的面试刷题平台", 0, 404, 31, 0x64748b, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_width(subtitle, 1280);

    lv_obj_t * stats = lv_obj_create(hero);
    lv_obj_remove_flag(stats, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(stats, 325, 506);
    lv_obj_set_size(stats, 630, 48);
    lv_obj_set_style_bg_opa(stats, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stats, 0, 0);
    lv_obj_set_style_radius(stats, 0, 0);
    lv_obj_set_style_pad_all(stats, 0, 0);

    create_logo_icon(stats, 15, 10, 24, 24);
    create_text(stats, "1000+", 48, 0, 32, 0x020817, LV_TEXT_ALIGN_LEFT);
    create_text(stats, "道企业真题", 132, 12, 19, 0x64748b, LV_TEXT_ALIGN_LEFT);

    create_company_icon(stats, 260, 8);
    create_text(stats, "140+", 300, 0, 32, 0x020817, LV_TEXT_ALIGN_LEFT);
    create_text(stats, "企业", 372, 12, 19, 0x64748b, LV_TEXT_ALIGN_LEFT);

    create_text(stats, "↗", 490, 5, 31, 0x2563eb, LV_TEXT_ALIGN_LEFT);
    create_text(stats, "60,000+", 526, 0, 32, 0x020817, LV_TEXT_ALIGN_LEFT);
    create_text(stats, "练习次数", 642, 12, 19, 0x64748b, LV_TEXT_ALIGN_LEFT);

    lv_obj_t * note = create_text(hero, "真题来自小智学长50个200+学习交流群成员面试记录",
                                 0, 585, 21, 0x64748b, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_width(note, 1280);

    lv_obj_t * cta = lv_button_create(hero);
    lv_obj_set_pos(cta, 560, 682);
    lv_obj_set_size(cta, 156, 62);
    lv_obj_set_style_radius(cta, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cta, lv_color_hex(0x2563eb), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(cta, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(cta, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(cta, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(cta, lv_color_hex(0x2563eb), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(cta, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cta, 0, LV_PART_MAIN);

    lv_obj_t * cta_label = lv_label_create(cta);
    lv_label_set_text(cta_label, "开始刷题  →");
    lv_obj_set_style_text_font(cta_label, ui_font_get(21), 0);
    lv_obj_set_style_text_color(cta_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(cta_label);

    return screen;
}

lv_obj_t * web_1858f708_page_get_content_root(void)
{
    return g_content_root;
}
