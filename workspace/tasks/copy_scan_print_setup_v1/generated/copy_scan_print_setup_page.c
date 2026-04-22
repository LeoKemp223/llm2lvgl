#include "copy_scan_print_setup_page.h"

#include "ui_font.h"

static lv_obj_t * g_content_root = NULL;

static void style_panel(lv_obj_t * obj, lv_color_t color, lv_opa_t opa, lv_coord_t radius)
{
    lv_obj_remove_style_all(obj);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
}

static lv_obj_t * make_icon_box(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_coord_t radius)
{
    lv_obj_t * obj = lv_obj_create(parent);
    style_panel(obj, lv_color_hex(0xffffff), LV_OPA_TRANSP, radius);
    lv_obj_set_style_border_color(obj, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    return obj;
}

static lv_obj_t * make_line(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t color, lv_coord_t radius)
{
    lv_obj_t * obj = lv_obj_create(parent);
    style_panel(obj, color, LV_OPA_COVER, radius);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    return obj;
}

static void make_copy_icon(lv_obj_t * parent)
{
    make_icon_box(parent, 0, 8, 18, 22, 4);
    lv_obj_t * back = make_icon_box(parent, 8, 0, 18, 22, 4);
    make_line(back, 10, 0, 6, 2, lv_color_hex(0xffffff), 1);
    make_line(back, 10, 5, 6, 2, lv_color_hex(0xffffff), 1);
    make_line(back, 10, 10, 6, 2, lv_color_hex(0xffffff), 1);
    make_line(back, 10, 15, 5, 2, lv_color_hex(0xffffff), 1);
    lv_obj_t * fold_v = make_line(back, 14, 0, 2, 6, lv_color_hex(0xffffff), 1);
    lv_obj_set_style_transform_rotation(fold_v, 450, 0);
}

static void make_scan_icon(lv_obj_t * parent)
{
    make_line(parent, 2, 0, 24, 4, lv_color_hex(0xffffff), 2);
    make_line(parent, 4, 3, 20, 5, lv_color_hex(0xffffff), 2);
    make_line(parent, 6, 8, 16, 7, lv_color_hex(0xffffff), 2);
    make_line(parent, 1, 17, 26, 4, lv_color_hex(0xffffff), 2);
    make_line(parent, 4, 20, 20, 5, lv_color_hex(0xffffff), 2);
}

static void make_print_icon(lv_obj_t * parent)
{
    make_icon_box(parent, 6, 0, 18, 10, 2);
    make_line(parent, 2, 8, 26, 11, lv_color_hex(0xffffff), 4);
    make_icon_box(parent, 7, 15, 16, 10, 2);
    make_line(parent, 21, 12, 3, 3, lv_color_hex(0x48b64b), 1);
}

static void make_setup_icon(lv_obj_t * parent)
{
    lv_obj_t * ring = lv_arc_create(parent);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, 22, 22);
    lv_obj_set_pos(ring, 3, 2);
    lv_arc_set_range(ring, 0, 360);
    lv_arc_set_angles(ring, 0, 360);
    lv_obj_set_style_arc_width(ring, 2, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ring, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);

    make_line(parent, 11, 0, 2, 5, lv_color_hex(0xffffff), 1);
    make_line(parent, 11, 21, 2, 5, lv_color_hex(0xffffff), 1);
    make_line(parent, 0, 11, 5, 2, lv_color_hex(0xffffff), 1);
    make_line(parent, 21, 11, 5, 2, lv_color_hex(0xffffff), 1);
    make_line(parent, 4, 4, 3, 2, lv_color_hex(0xffffff), 1);
    make_line(parent, 19, 4, 3, 2, lv_color_hex(0xffffff), 1);
    make_line(parent, 4, 20, 3, 2, lv_color_hex(0xffffff), 1);
    make_line(parent, 19, 20, 3, 2, lv_color_hex(0xffffff), 1);
}

static void make_status_pill(lv_obj_t * parent, const char * text, lv_coord_t x)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, ui_font_get(12), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xf6f7fb), 0);
    lv_obj_set_pos(label, x, 16);
}

static lv_obj_t * make_action_card(
    lv_obj_t * parent,
    lv_coord_t x,
    lv_coord_t y,
    lv_color_t bg,
    lv_color_t grad,
    const char * title
)
{
    lv_obj_t * card = lv_obj_create(parent);
    style_panel(card, bg, LV_OPA_COVER, 14);
    lv_obj_set_size(card, 74, 92);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_grad_color(card, grad, 0);
    lv_obj_set_style_bg_grad_dir(card, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_shadow_width(card, 16, 0);
    lv_obj_set_style_shadow_opa(card, 28, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0xbbbfd1), 0);
    lv_obj_set_style_shadow_offset_y(card, 8, 0);

    lv_obj_t * label = lv_label_create(card);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, ui_font_get(15), 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 9, -8);

    return card;
}

lv_obj_t * copy_scan_print_setup_page_create(void)
{
    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_size(screen, 379, 232);

    lv_obj_t * header = lv_obj_create(screen);
    style_panel(header, lv_color_hex(0x303346), LV_OPA_COVER, 0);
    lv_obj_set_size(header, 379, 75);
    lv_obj_set_pos(header, 0, 0);

    make_status_pill(header, ")))", 20);
    make_status_pill(header, ")", 68);
    make_status_pill(header, "<>", 104);
    make_status_pill(header, "[]", 148);

    lv_obj_t * date = lv_label_create(header);
    lv_label_set_text(date, "22 April 2020 15:36");
    lv_obj_set_style_text_font(date, ui_font_get(15), 0);
    lv_obj_set_style_text_color(date, lv_color_hex(0xf6f7fb), 0);
    lv_obj_set_pos(date, 222, 16);

    lv_obj_t * card_panel = lv_obj_create(screen);
    style_panel(card_panel, lv_color_hex(0xf9f8f2), LV_OPA_COVER, 5);
    lv_obj_set_size(card_panel, 360, 128);
    lv_obj_set_pos(card_panel, 12, 47);
    lv_obj_set_style_shadow_width(card_panel, 12, 0);
    lv_obj_set_style_shadow_opa(card_panel, 10, 0);
    lv_obj_set_style_shadow_color(card_panel, lv_color_hex(0xcfd3dd), 0);
    lv_obj_set_style_shadow_offset_y(card_panel, 5, 0);

    lv_obj_t * copy = make_action_card(card_panel, 13, 15, lv_color_hex(0xc637d4), lv_color_hex(0xa92cb8), "COPY");
    lv_obj_t * scan = make_action_card(card_panel, 100, 15, lv_color_hex(0x5c63de), lv_color_hex(0x444abb), "SCAN");
    lv_obj_t * print = make_action_card(card_panel, 187, 15, lv_color_hex(0x49ca45), lv_color_hex(0x3cad37), "PRINT");
    lv_obj_t * setup = make_action_card(card_panel, 274, 15, lv_color_hex(0xff2b2b), lv_color_hex(0xe22323), "SETUP");

    lv_obj_t * copy_icon = lv_obj_create(copy);
    style_panel(copy_icon, lv_color_hex(0xffffff), LV_OPA_TRANSP, 0);
    lv_obj_set_size(copy_icon, 28, 26);
    lv_obj_set_pos(copy_icon, 20, 10);
    make_copy_icon(copy_icon);

    lv_obj_t * scan_icon = lv_obj_create(scan);
    style_panel(scan_icon, lv_color_hex(0xffffff), LV_OPA_TRANSP, 0);
    lv_obj_set_size(scan_icon, 28, 26);
    lv_obj_set_pos(scan_icon, 20, 10);
    make_scan_icon(scan_icon);

    lv_obj_t * print_icon = lv_obj_create(print);
    style_panel(print_icon, lv_color_hex(0xffffff), LV_OPA_TRANSP, 0);
    lv_obj_set_size(print_icon, 30, 26);
    lv_obj_set_pos(print_icon, 18, 10);
    make_print_icon(print_icon);

    lv_obj_t * setup_icon = lv_obj_create(setup);
    style_panel(setup_icon, lv_color_hex(0xffffff), LV_OPA_TRANSP, 0);
    lv_obj_set_size(setup_icon, 28, 28);
    lv_obj_set_pos(setup_icon, 20, 9);
    make_setup_icon(setup_icon);

    lv_obj_t * prompt = lv_obj_create(screen);
    style_panel(prompt, lv_color_hex(0xf8f7f2), LV_OPA_COVER, 3);
    lv_obj_set_size(prompt, 253, 41);
    lv_obj_set_pos(prompt, 12, 186);

    lv_obj_t * prompt_label = lv_label_create(prompt);
    lv_label_set_text(prompt_label, "What do you want to do today?");
    lv_obj_set_style_text_font(prompt_label, ui_font_get(15), 0);
    lv_obj_set_style_text_color(prompt_label, lv_color_hex(0x2b2f35), 0);
    lv_obj_center(prompt_label);

    lv_obj_t * dots = lv_obj_create(screen);
    style_panel(dots, lv_color_hex(0xf8f7f2), LV_OPA_TRANSP, 0);
    lv_obj_set_size(dots, 95, 28);
    lv_obj_set_pos(dots, 279, 193);

    make_line(dots, 4, 4, 13, 18, lv_color_hex(0x2fd3cc), 6);
    make_line(dots, 25, 12, 11, 7, lv_color_hex(0xe33ee4), 3);
    make_line(dots, 48, 3, 12, 20, lv_color_hex(0xf4ea13), 5);
    make_line(dots, 71, 13, 12, 4, lv_color_hex(0x30303a), 2);

    g_content_root = screen;
    return screen;
}

lv_obj_t * copy_scan_print_setup_page_get_content_root(void)
{
    return g_content_root;
}
