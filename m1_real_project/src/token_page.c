#include "token_page.h"

#include <stdbool.h>

#include "ui_font.h"

typedef struct {
    lv_coord_t screen_width;
    lv_coord_t screen_height;
    lv_coord_t header_height;
    lv_coord_t hero_top;
    lv_coord_t content_width;
    lv_coord_t center_x;
} token_layout_t;

static const char * nav_items[] = {
    "首页",
    "控制台",
    "模型广场",
    "文档",
    "关于",
};

static token_layout_t layout;
static lv_obj_t * content_root = NULL;

static lv_style_t style_screen;
static lv_style_t style_header_text;
static lv_style_t style_nav_text;
static lv_style_t style_muted_text;
static lv_style_t style_brand_text;
static lv_style_t style_hero_text;
static lv_style_t style_subtitle_text;
static lv_style_t style_url_bar;
static lv_style_t style_url_text;
static lv_style_t style_url_path_text;
static lv_style_t style_primary_button;
static lv_style_t style_primary_button_text;
static lv_style_t style_secondary_button;
static lv_style_t style_secondary_button_text;
static lv_style_t style_icon_button;
static lv_style_t style_icon_text;
static lv_style_t style_login_button;
static lv_style_t style_heading_text;
static lv_style_t style_logo_text;
static bool styles_ready = false;

static const lv_font_t * icon_font(void)
{
    return LV_FONT_DEFAULT;
}

static void compute_layout(void)
{
    lv_display_t * display = lv_display_get_default();
    layout.screen_width = 1280;
    layout.screen_height = 800;

    if(display != NULL) {
        layout.screen_width = lv_display_get_horizontal_resolution(display);
        layout.screen_height = lv_display_get_vertical_resolution(display);
    }

    layout.header_height = 86;
    layout.hero_top = 118;
    layout.content_width = layout.screen_width - 80;
    layout.center_x = layout.screen_width / 2;
}

static void init_styles(void)
{
    if(styles_ready) {
        return;
    }

    const lv_font_t * small_font = ui_font_get(16);
    const lv_font_t * body_font = ui_font_get(18);
    const lv_font_t * nav_font = ui_font_get(22);
    const lv_font_t * title_font = ui_font_get(30);
    const lv_font_t * hero_font = ui_font_get(72);

    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);

    lv_style_init(&style_brand_text);
    lv_style_set_text_font(&style_brand_text, title_font);
    lv_style_set_text_color(&style_brand_text, lv_color_hex(0x111827));

    lv_style_init(&style_header_text);
    lv_style_set_text_font(&style_header_text, small_font);
    lv_style_set_text_color(&style_header_text, lv_color_hex(0x6b7280));

    lv_style_init(&style_nav_text);
    lv_style_set_text_font(&style_nav_text, nav_font);
    lv_style_set_text_color(&style_nav_text, lv_color_hex(0x111827));

    lv_style_init(&style_muted_text);
    lv_style_set_text_font(&style_muted_text, body_font);
    lv_style_set_text_color(&style_muted_text, lv_color_hex(0x6b7280));

    lv_style_init(&style_hero_text);
    lv_style_set_text_font(&style_hero_text, hero_font);
    lv_style_set_text_color(&style_hero_text, lv_color_hex(0x111827));

    lv_style_init(&style_subtitle_text);
    lv_style_set_text_font(&style_subtitle_text, ui_font_get(26));
    lv_style_set_text_color(&style_subtitle_text, lv_color_hex(0x4b5563));

    lv_style_init(&style_url_bar);
    lv_style_set_bg_color(&style_url_bar, lv_color_hex(0xf1f5f9));
    lv_style_set_bg_opa(&style_url_bar, LV_OPA_COVER);
    lv_style_set_radius(&style_url_bar, 28);
    lv_style_set_pad_hor(&style_url_bar, 24);
    lv_style_set_pad_ver(&style_url_bar, 18);
    lv_style_set_shadow_width(&style_url_bar, 20);
    lv_style_set_shadow_opa(&style_url_bar, 14);
    lv_style_set_shadow_color(&style_url_bar, lv_color_hex(0xdbeafe));
    lv_style_set_border_width(&style_url_bar, 0);

    lv_style_init(&style_url_text);
    lv_style_set_text_font(&style_url_text, ui_font_get(24));
    lv_style_set_text_color(&style_url_text, lv_color_hex(0x1f2937));

    lv_style_init(&style_url_path_text);
    lv_style_set_text_font(&style_url_path_text, ui_font_get(24));
    lv_style_set_text_color(&style_url_path_text, lv_color_hex(0x2563eb));

    lv_style_init(&style_primary_button);
    lv_style_set_bg_color(&style_primary_button, lv_color_hex(0x2563eb));
    lv_style_set_bg_opa(&style_primary_button, LV_OPA_COVER);
    lv_style_set_radius(&style_primary_button, 24);
    lv_style_set_pad_hor(&style_primary_button, 28);
    lv_style_set_pad_ver(&style_primary_button, 16);
    lv_style_set_shadow_width(&style_primary_button, 18);
    lv_style_set_shadow_opa(&style_primary_button, 18);
    lv_style_set_shadow_color(&style_primary_button, lv_color_hex(0x93c5fd));

    lv_style_init(&style_primary_button_text);
    lv_style_set_text_font(&style_primary_button_text, ui_font_get(24));
    lv_style_set_text_color(&style_primary_button_text, lv_color_hex(0xffffff));

    lv_style_init(&style_secondary_button);
    lv_style_set_bg_color(&style_secondary_button, lv_color_hex(0xf1f5f9));
    lv_style_set_bg_opa(&style_secondary_button, LV_OPA_COVER);
    lv_style_set_radius(&style_secondary_button, 24);
    lv_style_set_pad_hor(&style_secondary_button, 28);
    lv_style_set_pad_ver(&style_secondary_button, 16);
    lv_style_set_border_width(&style_secondary_button, 0);

    lv_style_init(&style_secondary_button_text);
    lv_style_set_text_font(&style_secondary_button_text, ui_font_get(24));
    lv_style_set_text_color(&style_secondary_button_text, lv_color_hex(0x2563eb));

    lv_style_init(&style_icon_button);
    lv_style_set_bg_color(&style_icon_button, lv_color_hex(0xf3f4f6));
    lv_style_set_bg_opa(&style_icon_button, LV_OPA_COVER);
    lv_style_set_radius(&style_icon_button, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_icon_button, 0);
    lv_style_set_border_width(&style_icon_button, 0);

    lv_style_init(&style_icon_text);
    lv_style_set_text_font(&style_icon_text, ui_font_get(18));
    lv_style_set_text_color(&style_icon_text, lv_color_hex(0x4b5563));

    lv_style_init(&style_login_button);
    lv_style_set_bg_color(&style_login_button, lv_color_hex(0xf3f4f6));
    lv_style_set_bg_opa(&style_login_button, LV_OPA_COVER);
    lv_style_set_radius(&style_login_button, 24);
    lv_style_set_pad_hor(&style_login_button, 24);
    lv_style_set_pad_ver(&style_login_button, 16);

    lv_style_init(&style_heading_text);
    lv_style_set_text_font(&style_heading_text, ui_font_get(34));
    lv_style_set_text_color(&style_heading_text, lv_color_hex(0x6b7280));

    lv_style_init(&style_logo_text);
    lv_style_set_text_font(&style_logo_text, ui_font_get(20));
    lv_style_set_text_color(&style_logo_text, lv_color_hex(0xffffff));

    styles_ready = true;
}

static lv_obj_t * create_label(lv_obj_t * parent, const char * text, const lv_style_t * style)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, (lv_style_t *)style, 0);
    return label;
}

static lv_obj_t * create_block(lv_obj_t * parent, lv_coord_t width, lv_coord_t height)
{
    lv_obj_t * block = lv_obj_create(parent);
    lv_obj_remove_style_all(block);
    lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(block, width, height);
    return block;
}

static lv_obj_t * create_row(lv_obj_t * parent, lv_coord_t width, lv_coord_t height, lv_coord_t gap)
{
    lv_obj_t * row = create_block(parent, width, height);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, gap, 0);
    return row;
}

static void create_circle(lv_obj_t * parent, lv_coord_t x, lv_coord_t y, lv_coord_t size, uint32_t color, lv_opa_t opa)
{
    lv_obj_t * circle = create_block(parent, size, size);
    lv_obj_set_pos(circle, x, y);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(circle, opa, 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    lv_obj_set_style_shadow_width(circle, size / 2, 0);
    lv_obj_set_style_shadow_opa(circle, opa / 2, 0);
    lv_obj_set_style_shadow_color(circle, lv_color_hex(color), 0);
}

static void create_top_bar(lv_obj_t * parent)
{
    lv_obj_t * header = create_block(parent, layout.screen_width, layout.header_height);
    lv_obj_set_pos(header, 0, 0);

    create_circle(header, 28, 22, 40, 0x22d3ee, LV_OPA_70);
    create_circle(header, 42, 22, 40, 0xec4899, LV_OPA_70);
    create_circle(header, 35, 29, 26, 0xffffff, LV_OPA_COVER);

    lv_obj_t * brand = create_label(header, "小黑词元", &style_brand_text);
    lv_obj_set_pos(brand, 92, 18);

    lv_coord_t nav_x = 260;
    for(size_t i = 0; i < sizeof(nav_items) / sizeof(nav_items[0]); ++i) {
        lv_obj_t * nav = create_label(header, nav_items[i], &style_nav_text);
        lv_obj_set_pos(nav, nav_x, 20);
        nav_x += 112;
    }

    const char * icon_texts[] = { LV_SYMBOL_BELL, LV_SYMBOL_SETTINGS, "A", "" };
    for(size_t i = 0; i < 4; ++i) {
        lv_obj_t * icon = create_block(header, 40, 40);
        lv_obj_set_pos(icon, layout.screen_width - 352 + (lv_coord_t)i * 64, 18);
        lv_obj_add_style(icon, &style_icon_button, 0);

        if(i < 3) {
            lv_obj_t * icon_label = create_label(icon, icon_texts[i], &style_icon_text);
            lv_obj_set_style_text_font(icon_label, icon_font(), 0);
            lv_obj_center(icon_label);
        }
        else {
            create_circle(icon, 6, 6, 28, 0xe5e7eb, LV_OPA_COVER);
        }
    }

    lv_obj_t * login = create_block(header, 76, 40);
    lv_obj_set_pos(login, layout.screen_width - 180, 18);
    lv_obj_add_style(login, &style_login_button, 0);
    lv_obj_t * login_text = create_label(login, "登录", &style_nav_text);
    lv_obj_center(login_text);
    lv_obj_set_style_text_font(login_text, ui_font_get(18), 0);

    lv_obj_t * register_button = create_block(header, 76, 40);
    lv_obj_set_pos(register_button, layout.screen_width - 96, 18);
    lv_obj_add_style(register_button, &style_primary_button, 0);
    lv_obj_set_style_radius(register_button, 20, 0);
    lv_obj_set_style_pad_all(register_button, 0, 0);
    lv_obj_set_style_shadow_width(register_button, 0, 0);
    lv_obj_t * register_text = create_label(register_button, "注册", &style_primary_button_text);
    lv_obj_center(register_text);
    lv_obj_set_style_text_font(register_text, ui_font_get(18), 0);
}

static void create_hero(lv_obj_t * parent)
{
    create_circle(parent, layout.center_x - 220, 110, 280, 0xbfdbfe, LV_OPA_50);
    create_circle(parent, layout.center_x - 90, 86, 320, 0xc7d2fe, LV_OPA_30);
    create_circle(parent, layout.center_x - 160, 160, 360, 0xd9f99d, LV_OPA_20);

    lv_obj_t * title = create_label(parent, "统一的\n大模型接口网关", &style_hero_text);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(title, 900);
    lv_obj_set_pos(title, layout.center_x - 450, layout.hero_top);

    lv_obj_t * glow = create_label(parent, "网关", &style_hero_text);
    lv_obj_set_style_text_color(glow, lv_color_hex(0xbfdbfe), 0);
    lv_obj_set_style_text_opa(glow, LV_OPA_40, 0);
    lv_obj_set_pos(glow, layout.center_x + 170, layout.hero_top + 168);

    lv_obj_t * subtitle = create_label(parent,
                                       "更好的价格，更好的稳定性，只需要将模型基址替换为：",
                                       &style_subtitle_text);
    lv_obj_set_pos(subtitle, layout.center_x - 260, layout.hero_top + 258);

    lv_obj_t * url_bar = create_block(parent, 620, 56);
    lv_obj_set_pos(url_bar, layout.center_x - 310, layout.hero_top + 334);
    lv_obj_add_style(url_bar, &style_url_bar, 0);
    lv_obj_set_layout(url_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(url_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(url_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(url_bar, 24, 0);
    lv_obj_set_style_pad_right(url_bar, 12, 0);
    lv_obj_set_style_pad_top(url_bar, 8, 0);
    lv_obj_set_style_pad_bottom(url_bar, 8, 0);

    lv_obj_t * url = create_label(url_bar, "https://token.766542.xyz", &style_url_text);
    lv_obj_set_style_text_font(url, ui_font_get(18), 0);
    lv_label_set_long_mode(url, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(url, 250);

    lv_obj_t * right_group = create_row(url_bar, 190, 40, 14);
    lv_obj_set_style_bg_opa(right_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(right_group, 0, 0);

    lv_obj_t * path = create_label(right_group, "/v1/messages", &style_url_path_text);
    lv_obj_set_style_text_font(path, ui_font_get(18), 0);
    lv_label_set_long_mode(path, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(path, 130);
    lv_obj_set_style_text_align(path, LV_TEXT_ALIGN_RIGHT, 0);

    lv_obj_t * copy_icon_box = create_block(right_group, 40, 40);
    lv_obj_add_style(copy_icon_box, &style_icon_button, 0);
    lv_obj_t * copy_icon = create_label(copy_icon_box, LV_SYMBOL_COPY, &style_icon_text);
    lv_obj_set_style_text_font(copy_icon, icon_font(), 0);
    lv_obj_center(copy_icon);

    lv_obj_t * primary = create_block(parent, 248, 64);
    lv_obj_set_pos(primary, layout.center_x - 118, layout.hero_top + 430);
    lv_obj_add_style(primary, &style_primary_button, 0);
    lv_obj_t * primary_row = create_row(primary, LV_SIZE_CONTENT, LV_SIZE_CONTENT, 10);
    lv_obj_set_style_bg_opa(primary_row, LV_OPA_TRANSP, 0);
    lv_obj_center(primary_row);
    lv_obj_t * primary_icon = create_label(primary_row, LV_SYMBOL_PLAY, &style_primary_button_text);
    lv_obj_set_style_text_font(primary_icon, icon_font(), 0);
    create_label(primary_row, "获取密钥", &style_primary_button_text);

    lv_obj_t * secondary = create_block(parent, 170, 64);
    lv_obj_set_pos(secondary, layout.center_x + 136, layout.hero_top + 430);
    lv_obj_add_style(secondary, &style_secondary_button, 0);
    lv_obj_t * secondary_row = create_row(secondary, LV_SIZE_CONTENT, LV_SIZE_CONTENT, 8);
    lv_obj_set_style_bg_opa(secondary_row, LV_OPA_TRANSP, 0);
    lv_obj_center(secondary_row);
    lv_obj_t * secondary_icon = create_label(secondary_row, LV_SYMBOL_DIRECTORY, &style_secondary_button_text);
    lv_obj_set_style_text_font(secondary_icon, icon_font(), 0);
    create_label(secondary_row, "文档", &style_secondary_button_text);

    lv_obj_t * heading = create_label(parent, "支持众多的大模型供应商", &style_heading_text);
    lv_obj_set_pos(heading, layout.center_x - 210, layout.hero_top + 534);

}

static void create_provider_row(lv_obj_t * parent)
{
    lv_obj_t * logos = lv_image_create(parent);
    lv_image_set_src(logos, TOKEN_PROVIDER_ROW_IMAGE_PATH);
    lv_obj_align(logos, LV_ALIGN_TOP_MID, 0, layout.hero_top + 596);
}

lv_obj_t * token_page_create(void)
{
    compute_layout();
    init_styles();

    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_size(screen, layout.screen_width, layout.screen_height);

    create_top_bar(screen);
    create_hero(screen);
    create_provider_row(screen);

    content_root = screen;
    return screen;
}

lv_obj_t * token_page_get_content_root(void)
{
    return content_root;
}
