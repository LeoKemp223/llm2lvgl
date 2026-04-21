#include "image_converter_page.h"

#include "ui_font.h"

/* ── Color palette (extracted from lvgl.io CSS variables) ── */
#define CLR_PURPLE_900   0x2a174f   /* --color-purple-900-rgb: 42,23,79   */
#define CLR_PURPLE_800   0x3f2377   /* --color-purple-800-rgb: 63,35,119  */
#define CLR_PURPLE_600   0x683ac5   /* --color-purple-600-rgb: 104,58,197 */
#define CLR_PURPLE_500   0x9364f0   /* --color-purple-500-rgb: 147,100,240*/
#define CLR_PURPLE_200   0xd4c1f9   /* --color-purple-200-rgb: 212,193,249*/
#define CLR_PURPLE_100   0xe5dafb   /* --color-purple-100-rgb: 229,218,251*/
#define CLR_ORANGE_BASE  0xea9a50   /* --color-orange-base-rgb            */
#define CLR_WHITE        0xffffff
#define CLR_BLACK        0x000000
#define CLR_DARK         0x0e0e0e   /* --color-dark-base-rgb: 14,14,14   */
#define CLR_TEXT_BASE    0x2a3336   /* --color-primary: rgb(42,51,54)     */
#define CLR_TEXT_SEC     0x3f4a4e   /* --color-secondary: rgb(63,74,78)   */
#define CLR_TEXT_TERT    0x8a9a9d   /* --color-tertiary: rgb(138,154,157) */
#define CLR_BG_PRI       0xf5f7f8   /* --bg-color-primary                */
#define CLR_BG_SEC       0xe9edee   /* --bg-color-secondary              */
#define CLR_BORDER       0xd9d9d9   /* rgba(0,0,0,0.16) on white ≈       */
#define CLR_BORDER_LIGHT 0xe8e8e8   /* rgba(0,0,0,0.08) on white         */
#define CLR_ORANGE_TIP_BG 0x2f1f10  /* [data-theme=orange] bg base       */
#define CLR_ORANGE_TIP_FG 0xea9a50

/* Radius constants (from CSS --radius-*) */
#define R_SM   8    /* --radius-sm: 0.5rem   */
#define R_MD  12    /* --radius-md: 0.75rem  */
#define R_LG  16    /* --radius-lg: 1rem     */
#define R_XL  24    /* --radius-xl: 1.5rem   */
#define R_PILL 200  /* --radius-full: 9999px */

static lv_obj_t * g_content_root = NULL;

static void create_nav_bar(lv_obj_t * parent, lv_coord_t w);
static void create_hero_section(lv_obj_t * parent, lv_coord_t w);
static void create_faq_section(lv_obj_t * parent, lv_coord_t w);
/* ── Helpers ── */
static lv_obj_t * mk_label(lv_obj_t * p, const char * txt, int sz, uint32_t clr)
{
    lv_obj_t * l = lv_label_create(p);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_font(l, ui_font_get(sz), 0);
    lv_obj_set_style_text_color(l, lv_color_hex(clr), 0);
    lv_obj_set_style_text_line_space(l, 4, 0);
    return l;
}

static lv_obj_t * mk_dropdown(lv_obj_t * p, const char * label_txt,
                               const char * options, lv_coord_t w)
{
    lv_obj_t * wrap = lv_obj_create(p);
    lv_obj_remove_style_all(wrap);
    lv_obj_remove_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(wrap, w);
    lv_obj_set_height(wrap, LV_SIZE_CONTENT);
    lv_obj_set_layout(wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(wrap, 8, 0);

    lv_obj_t * lbl = mk_label(wrap, label_txt, 14, CLR_TEXT_SEC);
    lv_obj_set_style_text_font(lbl, ui_font_get(14), 0);
    (void)lbl;

    lv_obj_t * dd = lv_dropdown_create(wrap);
    lv_dropdown_set_options(dd, options);
    lv_obj_set_width(dd, w);
    lv_obj_set_style_border_color(dd, lv_color_hex(CLR_BORDER), 0);
    lv_obj_set_style_border_width(dd, 1, 0);
    lv_obj_set_style_radius(dd, R_SM, 0);
    lv_obj_set_style_pad_all(dd, 14, 0);
    lv_obj_set_style_bg_color(dd, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_bg_opa(dd, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(dd, ui_font_get(15), 0);
    lv_obj_set_style_text_color(dd, lv_color_hex(CLR_TEXT_BASE), 0);
    return dd;
}

static lv_obj_t * mk_btn_primary(lv_obj_t * p, const char * txt, lv_coord_t w)
{
    lv_obj_t * btn = lv_button_create(p);
    lv_obj_set_width(btn, w);
    lv_obj_set_height(btn, 52);
    lv_obj_set_style_bg_color(btn, lv_color_hex(CLR_PURPLE_900), 0);
    lv_obj_set_style_radius(btn, R_PILL, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t * lbl = lv_label_create(btn);
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_text_font(lbl, ui_font_get(16), 0);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t * mk_file_btn(lv_obj_t * p, lv_coord_t w)
{
    lv_obj_t * row = lv_obj_create(p);
    lv_obj_remove_style_all(row);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, w);
    lv_obj_set_height(row, 48);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 16, 0);
    lv_obj_t * btn = lv_button_create(row);
    lv_obj_set_height(btn, 44);
    lv_obj_set_style_bg_color(btn, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_radius(btn, R_PILL, 0);
    lv_obj_set_style_pad_hor(btn, 20, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(CLR_BORDER), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_t * btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Select image file(s)");
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(CLR_TEXT_BASE), 0);
    lv_obj_set_style_text_font(btn_lbl, ui_font_get(14), 0);
    lv_obj_center(btn_lbl);

    mk_label(row, "No file chosen", 15, CLR_TEXT_TERT);
    return row;
}

static lv_obj_t * mk_checkbox(lv_obj_t * p, const char * txt, lv_coord_t w)
{
    lv_obj_t * cb = lv_checkbox_create(p);
    lv_checkbox_set_text(cb, txt);
    lv_obj_set_style_text_font(cb, ui_font_get(15), 0);
    lv_obj_set_style_text_color(cb, lv_color_hex(CLR_TEXT_BASE), 0);
    lv_obj_set_width(cb, w);
    return cb;
}

/* ══════════════════════════════════════════════════════════════
 *  Navigation bar
 * ══════════════════════════════════════════════════════════════ */
static void create_nav_bar(lv_obj_t * parent, lv_coord_t w)
{
    lv_obj_t * nav = lv_obj_create(parent);
    lv_obj_remove_style_all(nav);
    lv_obj_remove_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(nav, w, 64);
    lv_obj_set_style_bg_color(nav, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_bg_opa(nav, 210, 0);  /* rgba(255,255,255,0.8) */
    lv_obj_set_style_pad_hor(nav, 40, 0);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(nav, 24, 0);

    /* LVGL logo */
    lv_obj_t * logo = mk_label(nav, "LVGL", 18, CLR_DARK);
    lv_obj_set_style_pad_right(logo, 16, 0);

    /* Nav links */
    static const char * links[] = {
        "LVGL Open", "LVGL Safe", "LVGL Pro", "Blog", "Services",
    };
    for(int i = 0; i < 5; i++) {
        mk_label(nav, links[i], 14, CLR_TEXT_SEC);
    }

    /* Spacer to push Contact Us to right */
    lv_obj_t * spacer = lv_obj_create(nav);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_height(spacer, 1);

    /* GitHub button (secondary) */
    lv_obj_t * gh = lv_button_create(nav);
    lv_obj_set_style_bg_color(gh, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_radius(gh, R_PILL, 0);
    lv_obj_set_style_pad_hor(gh, 16, 0);
    lv_obj_set_style_pad_ver(gh, 8, 0);
    lv_obj_set_style_shadow_width(gh, 0, 0);
    lv_obj_set_style_border_color(gh, lv_color_hex(CLR_BORDER), 0);
    lv_obj_set_style_border_width(gh, 1, 0);
    lv_obj_t * gh_lbl = lv_label_create(gh);
    lv_label_set_text(gh_lbl, "LVGL on GitHub");
    lv_obj_set_style_text_color(gh_lbl, lv_color_hex(CLR_TEXT_BASE), 0);
    lv_obj_set_style_text_font(gh_lbl, ui_font_get(12), 0);
    lv_obj_center(gh_lbl);

    /* Contact Us button */
    lv_obj_t * cta = lv_button_create(nav);
    lv_obj_set_style_bg_color(cta, lv_color_hex(CLR_PURPLE_900), 0);
    lv_obj_set_style_radius(cta, R_PILL, 0);
    lv_obj_set_style_pad_hor(cta, 20, 0);
    lv_obj_set_style_pad_ver(cta, 10, 0);
    lv_obj_set_style_shadow_width(cta, 0, 0);
    lv_obj_t * cta_lbl = lv_label_create(cta);
    lv_label_set_text(cta_lbl, "Contact Us");
    lv_obj_set_style_text_color(cta_lbl, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_text_font(cta_lbl, ui_font_get(13), 0);
    lv_obj_center(cta_lbl);
}
/* ══════════════════════════════════════════════════════════════
 *  Hero section — purple bg, left title, right card
 * ══════════════════════════════════════════════════════════════ */
static lv_obj_t * create_tab_btn(lv_obj_t * parent, const char * txt,
                                  bool active, lv_coord_t w)
{
    lv_obj_t * btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(btn, w, 48);

    lv_obj_t * lbl = mk_label(btn, txt, 15,
                               active ? CLR_PURPLE_600 : CLR_TEXT_TERT);
    lv_obj_center(lbl);

    if(active) {
        lv_obj_set_style_border_color(btn, lv_color_hex(CLR_PURPLE_600), 0);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(btn, 3, 0);
    }
    return btn;
}

static void create_v9_form(lv_obj_t * parent, lv_coord_t fw)
{
    /* Orange tip box — [data-theme=orange] */
    lv_obj_t * tip = lv_obj_create(parent);
    lv_obj_remove_style_all(tip);
    lv_obj_remove_flag(tip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(tip, fw);
    lv_obj_set_height(tip, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tip, lv_color_hex(0xfff7ed), 0);
    lv_obj_set_style_bg_opa(tip, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(tip, R_LG, 0);
    lv_obj_set_style_pad_all(tip, 16, 0);

    lv_obj_t * tip_txt = mk_label(tip,
        "For image compression, stride, binary output,\n"
        "and extra color formats check out the\n"
        "Python image converter.", 14, 0x92400e);
    lv_label_set_long_mode(tip_txt, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(tip_txt, fw - 32);

    mk_file_btn(parent, fw);
    mk_dropdown(parent, "Color format",
                "RGB565\nRGB565A8\nRGB888\nXRGB8888\nARGB8888", fw);
    mk_btn_primary(parent, "Convert", fw);
}

static void create_v8_form(lv_obj_t * parent, lv_coord_t fw)
{
    mk_file_btn(parent, fw);
    mk_dropdown(parent, "Color format",
                "CF_ALPHA_1_BIT\nCF_ALPHA_2_BIT\nCF_ALPHA_4_BIT\n"
                "CF_ALPHA_8_BIT\nCF_INDEXED_1_BIT\nCF_INDEXED_2_BIT\n"
                "CF_INDEXED_4_BIT\nCF_INDEXED_8_BIT\nCF_RAW\n"
                "CF_RAW_CHROMA\nCF_RAW_ALPHA\nCF_TRUE_COLOR\n"
                "CF_TRUE_COLOR_ALPHA\nCF_TRUE_COLOR_CHROMA\nCF_RGB565A8",
                fw);

    lv_obj_t * note = mk_label(parent,
        "Alpha byte  Add an 8-bit Alpha value to every pixel\n"
        "Chroma keyed  Make LV_COLOR_TRANSP pixels transparent",
        13, CLR_TEXT_TERT);
    lv_label_set_long_mode(note, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(note, fw);

    mk_dropdown(parent, "Output format",
                "C array\nBinary RGB332\nBinary RGB565\n"
                "Binary RGB565 Swap\nBinary RGB888", fw);

    mk_checkbox(parent, "Dither images (can improve quality)", fw);
    mk_checkbox(parent, "Output in big-endian format", fw);
    mk_btn_primary(parent, "Convert", fw);
}
static void create_hero_section(lv_obj_t * parent, lv_coord_t w)
{
    /* Get viewport height for fill-view behavior */
    lv_coord_t vh = 800;
    lv_display_t * d = lv_display_get_default();
    if(d) vh = lv_display_get_vertical_resolution(d);
    lv_coord_t hero_h = vh - 64; /* subtract nav height */

    /* Full-width purple background */
    lv_obj_t * hero = lv_obj_create(parent);
    lv_obj_remove_style_all(hero);
    lv_obj_remove_flag(hero, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(hero, w, hero_h);
    lv_obj_set_style_bg_color(hero, lv_color_hex(CLR_PURPLE_900), 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(hero, 48, 0);
    lv_obj_set_style_pad_bottom(hero, 48, 0);
    lv_obj_set_style_pad_hor(hero, 80, 0);

    /* Inner wrapper: grid 1fr / 2fr, vertically centered */
    lv_obj_t * wrap = lv_obj_create(hero);
    lv_obj_remove_style_all(wrap);
    lv_obj_remove_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(wrap, w - 160);
    lv_obj_set_height(wrap, LV_SIZE_CONTENT);
    lv_obj_set_layout(wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wrap, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(wrap, 48, 0);
    lv_obj_align(wrap, LV_ALIGN_CENTER, 0, 0);

    lv_coord_t inner_w = w - 160;
    lv_coord_t aside_w = inner_w / 3;
    lv_coord_t card_w  = inner_w * 2 / 3 - 48;

    /* ── Left aside: title + subtitle ── */
    lv_obj_t * aside = lv_obj_create(wrap);
    lv_obj_remove_style_all(aside);
    lv_obj_remove_flag(aside, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(aside, aside_w);
    lv_obj_set_height(aside, LV_SIZE_CONTENT);
    lv_obj_set_layout(aside, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(aside, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(aside, 24, 0);
    lv_obj_set_style_pad_top(aside, 40, 0);

    /* H1: clamp(2.75em,4.16vw,3.75rem) → ~48px at 1280, line-height:100% */
    lv_obj_t * h1 = mk_label(aside, "Image\nConverter", 48, CLR_WHITE);
    lv_obj_set_style_text_line_space(h1, -8, 0);  /* tight line-height:100% */
    lv_obj_set_style_text_letter_space(h1, -3, 0); /* letter-spacing:-.06em */
    lv_label_set_long_mode(h1, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(h1, aside_w);

    lv_obj_t * sub = mk_label(aside,
        "Convert BMP, JPG, PNG, or SVG\nto C array to use them in LVGL.",
        18, CLR_PURPLE_200);
    lv_label_set_long_mode(sub, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(sub, aside_w);
    /* ── Right side: white converter card ── */
    lv_coord_t form_w = card_w - 56;

    lv_obj_t * card = lv_obj_create(wrap);
    lv_obj_remove_style_all(card);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(card, card_w);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, R_XL, 0);
    lv_obj_set_style_pad_all(card, 28, 0);
    lv_obj_set_style_shadow_width(card, 48, 0);
    lv_obj_set_style_shadow_opa(card, 40, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(CLR_BLACK), 0);
    lv_obj_set_style_shadow_offset_y(card, 8, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(card, 0, 0);

    /* Tab bar */
    lv_obj_t * tab_bar = lv_obj_create(card);
    lv_obj_remove_style_all(tab_bar);
    lv_obj_remove_flag(tab_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(tab_bar, form_w);
    lv_obj_set_height(tab_bar, 50);
    lv_obj_set_layout(tab_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tab_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_border_color(tab_bar, lv_color_hex(CLR_BORDER_LIGHT), 0);
    lv_obj_set_style_border_side(tab_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(tab_bar, 1, 0);

    create_tab_btn(tab_bar, "LVGL v9", true, form_w / 2);
    create_tab_btn(tab_bar, "LVGL v8", false, form_w / 2);

    /* V9 form (visible) */
    lv_obj_t * v9 = lv_obj_create(card);
    lv_obj_remove_style_all(v9);
    lv_obj_remove_flag(v9, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(v9, form_w);
    lv_obj_set_height(v9, LV_SIZE_CONTENT);
    lv_obj_set_layout(v9, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(v9, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(v9, 20, 0);
    lv_obj_set_style_pad_top(v9, 24, 0);
    create_v9_form(v9, form_w);

    /* V8 form (hidden) */
    lv_obj_t * v8 = lv_obj_create(card);
    lv_obj_remove_style_all(v8);
    lv_obj_remove_flag(v8, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(v8, form_w);
    lv_obj_set_height(v8, LV_SIZE_CONTENT);
    lv_obj_set_layout(v8, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(v8, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(v8, 20, 0);
    lv_obj_set_style_pad_top(v8, 24, 0);
    lv_obj_add_flag(v8, LV_OBJ_FLAG_HIDDEN);
    create_v8_form(v8, form_w);
}
/* ══════════════════════════════════════════════════════════════
 *  FAQ section — white bg, grid 1fr/2fr, rounded card items
 * ══════════════════════════════════════════════════════════════ */
static lv_obj_t * mk_faq_item(lv_obj_t * parent, const char * q,
                               const char * a, lv_coord_t w, bool expanded)
{
    lv_obj_t * item = lv_obj_create(parent);
    lv_obj_remove_style_all(item);
    lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(item, w);
    lv_obj_set_height(item, LV_SIZE_CONTENT);
    lv_obj_set_layout(item, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(item, 24, 0);
    lv_obj_set_style_pad_row(item, 12, 0);
    lv_obj_set_style_bg_color(item, lv_color_hex(CLR_BG_PRI), 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(item, R_XL, 0);

    /* Question row */
    lv_obj_t * qrow = lv_obj_create(item);
    lv_obj_remove_style_all(qrow);
    lv_obj_remove_flag(qrow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(qrow, w - 48);
    lv_obj_set_height(qrow, LV_SIZE_CONTENT);
    lv_obj_set_layout(qrow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(qrow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(qrow, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * ql = mk_label(qrow, q, 18, CLR_TEXT_BASE);
    lv_label_set_long_mode(ql, LV_LABEL_LONG_WRAP);
    lv_obj_set_flex_grow(ql, 1);

    /* Toggle indicator */
    lv_obj_t * arrow = mk_label(qrow, expanded ? LV_SYMBOL_UP : LV_SYMBOL_DOWN,
                                 14, CLR_TEXT_TERT);
    (void)arrow;

    /* Answer (only if expanded) */
    if(expanded) {
        lv_obj_t * al = mk_label(item, a, 15, CLR_TEXT_SEC);
        lv_label_set_long_mode(al, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(al, w - 48);
    }
    return item;
}

static void create_faq_section(lv_obj_t * parent, lv_coord_t w)
{
    lv_obj_t * faq = lv_obj_create(parent);
    lv_obj_remove_style_all(faq);
    lv_obj_remove_flag(faq, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(faq, w);
    lv_obj_set_height(faq, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(faq, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_bg_opa(faq, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_top(faq, 64, 0);
    lv_obj_set_style_pad_bottom(faq, 64, 0);
    lv_obj_set_style_pad_hor(faq, 80, 0);

    /* Inner grid: 1fr title | 2fr items */
    lv_obj_t * grid = lv_obj_create(faq);
    lv_obj_remove_style_all(grid);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(grid, w - 160);
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(grid, 48, 0);

    lv_coord_t inner_w = w - 160;
    lv_coord_t title_w = inner_w / 3;
    lv_coord_t items_w = inner_w * 2 / 3 - 48;

    /* Title — h2 style: clamp(2em,2.77vw,2.5em) → ~36px */
    lv_obj_t * title = mk_label(grid, "Frequently\nAsked\nQuestions",
                                 36, CLR_TEXT_BASE);
    lv_obj_set_style_text_line_space(title, -2, 0);
    lv_obj_set_style_text_letter_space(title, -2, 0);
    lv_obj_set_width(title, title_w);

    /* Items column */
    lv_obj_t * items = lv_obj_create(grid);
    lv_obj_remove_style_all(items);
    lv_obj_remove_flag(items, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(items, items_w);
    lv_obj_set_height(items, LV_SIZE_CONTENT);
    lv_obj_set_layout(items, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(items, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(items, 8, 0);

    mk_faq_item(items, "How to use the image converter?",
        "1. Choose one or more images\n"
        "2. Give a name to the output file(s)\n"
        "3. Specify the desired color format\n"
        "4. Click the Convert button to download",
        items_w, true);

    mk_faq_item(items, "How to use the generated file in LVGL?",
        "1. Copy the resulting C file into your project\n"
        "2. Declare: LV_IMG_DECLARE(my_image_name);\n"
        "3. Set source: lv_image_set_src(img, &my_image_name);\n"
        "4. See docs.lvgl.io for more details",
        items_w, false);
}
/* ══════════════════════════════════════════════════════════════
 *  Page entry points
 * ══════════════════════════════════════════════════════════════ */
lv_obj_t * image_converter_page_create(void)
{
    lv_coord_t sw = 1280;
    lv_display_t * d = lv_display_get_default();
    if(d) {
        sw = lv_display_get_horizontal_resolution(d);
    }

    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_set_size(screen, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(screen, lv_color_hex(CLR_WHITE), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_scroll_dir(screen, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t * content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(content, sw);
    lv_obj_set_height(content, LV_SIZE_CONTENT);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    create_nav_bar(content, sw);
    create_hero_section(content, sw);
    create_faq_section(content, sw);

    g_content_root = content;
    return screen;
}

lv_obj_t * image_converter_page_get_content_root(void)
{
    return g_content_root;
}
