#include "home_page.h"

#include <stdbool.h>
#include <stddef.h>

#include "ui_font.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

typedef struct {
    const char * title;
    const char * description;
    const char * badge;
    uint32_t accent;
} tool_card_t;

typedef struct {
    const char * title;
    uint32_t accent;
    const tool_card_t * cards;
    size_t card_count;
} tool_section_t;

static const tool_card_t hardware_cards[] = {
    { "PCB 过孔电流计算器", "基于 IPC-2152 标准，计算过孔最大电流、电阻、压降等参数", "V", 0xf59e0b },
    { "PCB 走线宽度计算器", "基于 IPC-2152 标准，根据电流计算所需走线宽度", "T", 0x2563eb },
    { "二进制文件查看器", "在线查看二进制文件，十六进制和 ASCII 视图，支持大文件虚拟滚动", "B", 0x0ea5e9 },
    { "电池续航计算器", "计算电池使用寿命，支持恒定功耗和间歇性功耗两种模式", "P", 0x10b981 },
    { "LED 限流电阻计算器", "计算 LED 限流电阻值，支持串并联配置，E24 标准电阻推荐", "L", 0xeab308 },
    { "电阻色环阻值计算器", "支持 4/5/6 环电阻的色环阻值、容差及温度系数计算", "R", 0xf97316 },
    { "SMD 电阻丝印助手", "解析三位、四位及 EIA-96 编码的贴片电阻丝印代码", "S", 0x059669 },
    { "RC/RL 滤波器计算器", "低通与高通滤波器计算，支持 RC 和 RL 电路模式，实时幅频响应曲线", "F", 0x3b82f6 },
    { "ESP32 在线烧录", "乐鑫官方 Web Flash 工具，在线烧录固件到 ESP32 系列芯片", "E", 0x10b981 },
    { "电路在线仿真", "在线电路仿真工具，支持 Arduino、ESP32、STM32 等多种开发板", "C", 0x7c3aed },
    { "芯片手册查询", "电子元器件 datasheet 数据手册查询下载，支持数百万种芯片规格书", "D", 0xea580c },
};

static const tool_card_t network_cards[] = {
    { "Web 串口助手", "基于 Web Serial API 的在线调试", "S", 0xdc2626 },
    { "HTTP 客户端", "支持常见 HTTP 请求方法，自定义请求头和参数", "H", 0x2563eb },
};

static const tool_card_t code_cards[] = {
    { "JSON 格式化", "美化、压缩、转义 JSON 数据", "J", 0x2563eb },
    { "代码对比", "实时按行对比两段代码，高亮显示差异", "D", 0x9333ea },
    { "日志查看器", "高性能日志分析，支持大文件、实时过滤和关键字高亮", "L", 0x2563eb },
    { "Markdown 编辑器", "实时预览与快捷编辑，支持导出 Markdown 和 HTML", "M", 0x4f46e5 },
};

static const tool_card_t ai_cards[] = {
    { "小智同学", "使用小智学长课程资料训练的专属答疑助教", "A", 0x0ea5e9 },
};

static const tool_card_t crc_cards[] = {
    { "CRC 计算", "CRC8/16/32, LRC, BCC 校验", "C", 0x16a34a },
    { "Hash / MD5 / SHA", "文件与文本哈希值在线计算", "H", 0x15803d },
    { "Base64 编解码", "Base64 标准编解码，支持 UTF-8", "B", 0x16a34a },
};

static const tool_card_t radix_cards[] = {
    { "进制互转", "2/8/10/16 进制及位域可视化", "N", 0x2563eb },
    { "ASCII / 字符", "ASCII 码与字符相互批量转换", "A", 0x4f46e5 },
};

static const tool_card_t time_cards[] = {
    { "时间戳转换", "Unix <-> 本地 / UTC, 倒计时", "T", 0xea580c },
    { "进制计算器", "支持 10 进制与 16 进制的专业计算器，实时切换显示", "C", 0xf97316 },
};

static const tool_card_t design_cards[] = {
    { "颜色选择器", "HEX/RGB/HSL/CMYK 颜色转换与配色方案生成", "P", 0xdb2777 },
    { "二维码生成器", "支持网址、文本、WiFi、名片、邮件、短信等多种类型，可自定义颜色和 Logo", "Q", 0x4f46e5 },
};

static const tool_section_t sections[] = {
    { "硬件/PCB", 0x6b7280, hardware_cards, ARRAY_SIZE(hardware_cards) },
    { "串口 / 网络调试 (轻量版)", 0xef4444, network_cards, ARRAY_SIZE(network_cards) },
    { "代码美化 / 转换", 0x6b7280, code_cards, ARRAY_SIZE(code_cards) },
    { "AI 助手", 0x6b7280, ai_cards, ARRAY_SIZE(ai_cards) },
    { "CRC / 校验计算", 0x22c55e, crc_cards, ARRAY_SIZE(crc_cards) },
    { "进制转换与位操作", 0x3b82f6, radix_cards, ARRAY_SIZE(radix_cards) },
    { "时间 / 计算器", 0xf97316, time_cards, ARRAY_SIZE(time_cards) },
    { "设计工具", 0x6b7280, design_cards, ARRAY_SIZE(design_cards) },
};

static lv_style_t style_screen;
static lv_style_t style_nav;
static lv_style_t style_sidebar;
static lv_style_t style_content;
static lv_style_t style_panel;
static lv_style_t style_card;
static lv_style_t style_sidebar_item;
static lv_style_t style_sidebar_item_active;
static lv_style_t style_nav_link;
static lv_style_t style_nav_link_active;
static lv_style_t style_title;
static lv_style_t style_primary_text;
static lv_style_t style_card_title;
static lv_style_t style_section_title;
static lv_style_t style_body;
static lv_style_t style_muted;
static lv_style_t style_footer;
static bool styles_ready = false;
static lv_obj_t * content_root = NULL;

typedef struct {
    lv_coord_t nav_height;
    lv_coord_t sidebar_width;
    lv_coord_t content_pad;
    lv_coord_t section_gap;
    lv_coord_t heading_height;
    lv_coord_t inner_gap;
    lv_coord_t card_gap;
    lv_coord_t card_width;
    lv_coord_t card_height;
    lv_coord_t grid_columns;
    lv_coord_t grid_width;
} layout_metrics_t;

static layout_metrics_t layout_metrics;

static void compute_layout(void)
{
    lv_display_t * display = lv_display_get_default();
    lv_coord_t screen_width = 1280;
    lv_coord_t screen_height = 800;

    if(display != NULL) {
        screen_width = lv_display_get_horizontal_resolution(display);
        screen_height = lv_display_get_vertical_resolution(display);
    }

    layout_metrics.nav_height = 58;
    layout_metrics.sidebar_width = screen_width >= 1280 ? 208 : 188;
    layout_metrics.content_pad = 20;
    layout_metrics.section_gap = 24;
    layout_metrics.heading_height = 26;
    layout_metrics.inner_gap = 12;
    layout_metrics.card_gap = 12;
    layout_metrics.grid_columns = 4;
    layout_metrics.card_width = 0;

    lv_coord_t available_width = screen_width - layout_metrics.sidebar_width - layout_metrics.content_pad * 2 - 2;
    while(layout_metrics.grid_columns > 1) {
        lv_coord_t candidate_width =
            (available_width - (layout_metrics.grid_columns - 1) * layout_metrics.card_gap) / layout_metrics.grid_columns;
        if(candidate_width >= 206) {
            layout_metrics.card_width = candidate_width;
            break;
        }

        layout_metrics.grid_columns--;
    }

    if(layout_metrics.card_width == 0) {
        layout_metrics.card_width = available_width;
    }

    if(layout_metrics.card_width > 220) {
        layout_metrics.card_width = 220;
    }
    if(layout_metrics.card_width < 196) {
        layout_metrics.card_width = 196;
    }

    if(screen_height >= 860) {
        layout_metrics.card_height = 156;
    }
    else if(screen_height >= 760) {
        layout_metrics.card_height = 148;
    }
    else {
        layout_metrics.card_height = 142;
    }

    layout_metrics.grid_width = layout_metrics.grid_columns * layout_metrics.card_width +
                                (layout_metrics.grid_columns - 1) * layout_metrics.card_gap;
}

static lv_coord_t get_grid_rows(size_t card_count)
{
    return (lv_coord_t)((card_count + (size_t)layout_metrics.grid_columns - 1U) / (size_t)layout_metrics.grid_columns);
}

static lv_coord_t get_grid_height(size_t card_count)
{
    lv_coord_t rows = get_grid_rows(card_count);
    if(rows == 0) {
        return 0;
    }

    return rows * layout_metrics.card_height + (rows - 1) * layout_metrics.card_gap;
}

static lv_coord_t get_section_height(size_t card_count)
{
    return layout_metrics.heading_height + layout_metrics.inner_gap + get_grid_height(card_count);
}

static void init_styles(void)
{
    if(styles_ready) {
        return;
    }

    const lv_font_t * body_font = ui_font_get(15);
    const lv_font_t * small_font = ui_font_get(13);
    const lv_font_t * title_font = ui_font_get(17);

    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0xf8fafc));
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_text_color(&style_screen, lv_color_hex(0x111827));
    lv_style_set_text_font(&style_screen, body_font);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_radius(&style_screen, 0);

    lv_style_init(&style_nav);
    lv_style_set_bg_color(&style_nav, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&style_nav, LV_OPA_COVER);
    lv_style_set_border_color(&style_nav, lv_color_hex(0xe5e7eb));
    lv_style_set_border_side(&style_nav, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&style_nav, 1);
    lv_style_set_pad_hor(&style_nav, 20);
    lv_style_set_pad_ver(&style_nav, 10);
    lv_style_set_radius(&style_nav, 0);

    lv_style_init(&style_sidebar);
    lv_style_set_bg_color(&style_sidebar, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&style_sidebar, LV_OPA_COVER);
    lv_style_set_border_color(&style_sidebar, lv_color_hex(0xe5e7eb));
    lv_style_set_border_side(&style_sidebar, LV_BORDER_SIDE_RIGHT);
    lv_style_set_border_width(&style_sidebar, 1);
    lv_style_set_pad_all(&style_sidebar, 18);
    lv_style_set_pad_row(&style_sidebar, 6);
    lv_style_set_radius(&style_sidebar, 0);

    lv_style_init(&style_content);
    lv_style_set_bg_color(&style_content, lv_color_hex(0xf8fafc));
    lv_style_set_bg_opa(&style_content, LV_OPA_COVER);
    lv_style_set_pad_all(&style_content, layout_metrics.content_pad);
    lv_style_set_pad_row(&style_content, layout_metrics.section_gap);
    lv_style_set_pad_column(&style_content, 0);
    lv_style_set_border_width(&style_content, 0);
    lv_style_set_radius(&style_content, 0);

    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_pad_all(&style_panel, 0);
    lv_style_set_border_width(&style_panel, 0);
    lv_style_set_radius(&style_panel, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0xffffff));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, lv_color_hex(0xe5e7eb));
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, 10);
    lv_style_set_pad_all(&style_card, 14);
    lv_style_set_pad_row(&style_card, 8);
    lv_style_set_shadow_width(&style_card, 8);
    lv_style_set_shadow_opa(&style_card, 12);
    lv_style_set_shadow_offset_y(&style_card, 2);
    lv_style_set_shadow_color(&style_card, lv_color_hex(0x94a3b8));

    lv_style_init(&style_sidebar_item);
    lv_style_set_bg_opa(&style_sidebar_item, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_sidebar_item, lv_color_hex(0x4b5563));
    lv_style_set_pad_hor(&style_sidebar_item, 12);
    lv_style_set_pad_ver(&style_sidebar_item, 8);
    lv_style_set_radius(&style_sidebar_item, 9);
    lv_style_set_border_width(&style_sidebar_item, 0);

    lv_style_init(&style_sidebar_item_active);
    lv_style_set_bg_color(&style_sidebar_item_active, lv_color_hex(0xdbeafe));
    lv_style_set_bg_opa(&style_sidebar_item_active, LV_OPA_COVER);
    lv_style_set_text_color(&style_sidebar_item_active, lv_color_hex(0x2563eb));

    lv_style_init(&style_nav_link);
    lv_style_set_bg_opa(&style_nav_link, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_nav_link, lv_color_hex(0x4b5563));
    lv_style_set_pad_hor(&style_nav_link, 10);
    lv_style_set_pad_ver(&style_nav_link, 7);
    lv_style_set_border_width(&style_nav_link, 0);
    lv_style_set_radius(&style_nav_link, 0);

    lv_style_init(&style_nav_link_active);
    lv_style_set_text_color(&style_nav_link_active, lv_color_hex(0x2563eb));
    lv_style_set_border_color(&style_nav_link_active, lv_color_hex(0x2563eb));
    lv_style_set_border_side(&style_nav_link_active, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_border_width(&style_nav_link_active, 2);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0x111827));
    lv_style_set_text_font(&style_title, title_font);

    lv_style_init(&style_primary_text);
    lv_style_set_text_color(&style_primary_text, lv_color_hex(0x2563eb));
    lv_style_set_text_font(&style_primary_text, body_font);

    lv_style_init(&style_card_title);
    lv_style_set_text_color(&style_card_title, lv_color_hex(0x111827));
    lv_style_set_text_font(&style_card_title, body_font);

    lv_style_init(&style_section_title);
    lv_style_set_text_color(&style_section_title, lv_color_hex(0x1f2937));
    lv_style_set_text_font(&style_section_title, title_font);

    lv_style_init(&style_body);
    lv_style_set_text_color(&style_body, lv_color_hex(0x111827));
    lv_style_set_text_font(&style_body, body_font);

    lv_style_init(&style_muted);
    lv_style_set_text_color(&style_muted, lv_color_hex(0x6b7280));
    lv_style_set_text_font(&style_muted, small_font);

    lv_style_init(&style_footer);
    lv_style_set_text_color(&style_footer, lv_color_hex(0x9ca3af));
    lv_style_set_text_font(&style_footer, small_font);

    styles_ready = true;
}

static void scroll_to_target_cb(lv_event_t * event)
{
    lv_obj_t * target = lv_event_get_user_data(event);

    if(target != NULL) {
        lv_obj_scroll_to_view(target, LV_ANIM_ON);
    }
}

static lv_obj_t * create_text_node(lv_obj_t * parent, const char * text, const lv_style_t * style)
{
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, (lv_style_t *)style, 0);
    return label;
}

static lv_obj_t * create_nav_link(lv_obj_t * parent, const char * text, bool active)
{
    lv_obj_t * link = lv_obj_create(parent);
    lv_obj_remove_style_all(link);
    lv_obj_remove_flag(link, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(link, &style_nav_link, 0);
    lv_obj_set_width(link, LV_SIZE_CONTENT);
    lv_obj_set_height(link, LV_SIZE_CONTENT);
    if(active) {
        lv_obj_add_style(link, &style_nav_link_active, 0);
    }

    create_text_node(link, text, active ? &style_primary_text : &style_body);
    return link;
}

static lv_obj_t * create_sidebar_item(lv_obj_t * parent, const char * text, lv_obj_t * target, bool active)
{
    lv_obj_t * item = lv_obj_create(parent);
    lv_obj_remove_style_all(item);
    lv_obj_remove_flag(item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(item, lv_pct(100));
    lv_obj_set_height(item, LV_SIZE_CONTENT);
    lv_obj_add_style(item, &style_sidebar_item, 0);
    if(active) {
        lv_obj_add_style(item, &style_sidebar_item_active, 0);
    }
    lv_obj_add_event_cb(item, scroll_to_target_cb, LV_EVENT_CLICKED, target);

    lv_obj_t * label = create_text_node(item, text, active ? &style_primary_text : &style_body);
    lv_obj_set_width(label, lv_pct(100));

    return item;
}

static lv_obj_t * create_card(lv_obj_t * parent, const tool_card_t * card)
{
    lv_obj_t * wrapper = lv_obj_create(parent);
    lv_obj_remove_style_all(wrapper);
    lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(wrapper, &style_card, 0);
    lv_obj_set_width(wrapper, layout_metrics.card_width);
    lv_obj_set_height(wrapper, layout_metrics.card_height);

    lv_obj_t * badge = lv_obj_create(wrapper);
    lv_obj_remove_style_all(badge);
    lv_obj_remove_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(badge, 26, 26);
    lv_obj_set_pos(badge, 14, 14);
    lv_obj_set_style_radius(badge, 8, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(card->accent), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(badge, 0, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);

    lv_obj_t * badge_text = lv_label_create(badge);
    lv_label_set_text(badge_text, card->badge);
    lv_obj_center(badge_text);
    lv_obj_set_style_text_color(badge_text, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(badge_text, ui_font_get(15), 0);

    lv_obj_t * title = lv_label_create(wrapper);
    lv_label_set_text(title, card->title);
    lv_obj_add_style(title, &style_card_title, 0);
    lv_label_set_long_mode(title, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(title, layout_metrics.card_width - 64);
    lv_obj_set_pos(title, 48, 10);
    lv_obj_set_style_text_line_space(title, 2, 0);

    lv_obj_t * description = lv_label_create(wrapper);
    lv_label_set_text(description, card->description);
    lv_obj_add_style(description, &style_muted, 0);
    lv_label_set_long_mode(description, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(description, layout_metrics.card_width - 28);
    lv_obj_set_pos(description, 14, 54);
    lv_obj_set_style_text_line_space(description, 2, 0);

    return wrapper;
}

static lv_obj_t * create_section(lv_obj_t * parent, const tool_section_t * section)
{
    lv_obj_t * container = lv_obj_create(parent);
    lv_obj_remove_style_all(container);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(container, lv_pct(100));
    lv_obj_set_height(container, get_section_height(section->card_count));

    lv_obj_t * heading = lv_obj_create(container);
    lv_obj_remove_style_all(heading);
    lv_obj_set_width(heading, lv_pct(100));
    lv_obj_set_height(heading, layout_metrics.heading_height);

    lv_obj_t * marker = lv_obj_create(heading);
    lv_obj_remove_style_all(marker);
    lv_obj_remove_flag(marker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(marker, 4, 18);
    lv_obj_set_pos(marker, 0, 4);
    lv_obj_set_style_radius(marker, 4, 0);
    lv_obj_set_style_bg_color(marker, lv_color_hex(section->accent), 0);
    lv_obj_set_style_bg_opa(marker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(marker, 0, 0);

    lv_obj_t * title = create_text_node(heading, section->title, &style_section_title);
    lv_obj_set_pos(title, 12, 1);

    lv_obj_t * grid = lv_obj_create(container);
    lv_obj_remove_style_all(grid);
    lv_obj_remove_flag(grid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(grid, layout_metrics.grid_width);
    lv_obj_set_height(grid, get_grid_height(section->card_count));
    lv_obj_set_pos(grid, 0, layout_metrics.heading_height + layout_metrics.inner_gap);

    for(size_t i = 0; i < section->card_count; ++i) {
        lv_obj_t * card = create_card(grid, &section->cards[i]);
        lv_coord_t col = (lv_coord_t)(i % (size_t)layout_metrics.grid_columns);
        lv_coord_t row = (lv_coord_t)(i / (size_t)layout_metrics.grid_columns);
        lv_obj_set_pos(card,
                       col * (layout_metrics.card_width + layout_metrics.card_gap),
                       row * (layout_metrics.card_height + layout_metrics.card_gap));
    }

    return container;
}

static void create_top_nav(lv_obj_t * parent)
{
    lv_obj_t * nav = lv_obj_create(parent);
    lv_obj_remove_style_all(nav);
    lv_obj_remove_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(nav, &style_nav, 0);
    lv_obj_set_width(nav, lv_pct(100));
    lv_obj_set_height(nav, layout_metrics.nav_height);
    lv_obj_set_layout(nav, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    create_text_node(nav, "小智学长工具集", &style_title);

    lv_obj_t * links = lv_obj_create(nav);
    lv_obj_remove_style_all(links);
    lv_obj_set_width(links, LV_SIZE_CONTENT);
    lv_obj_set_height(links, LV_SIZE_CONTENT);
    lv_obj_set_layout(links, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(links, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(links, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(links, 8, 0);

    create_nav_link(links, "在线工具", true);
    create_nav_link(links, "文档与资料中心", false);
    create_nav_link(links, "面包刷题", false);
}

lv_obj_t * home_page_create(void)
{
    compute_layout();
    init_styles();
    content_root = NULL;

    lv_obj_t * screen = lv_obj_create(NULL);
    lv_obj_remove_style_all(screen);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(screen, &style_screen, 0);
    lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);

    create_top_nav(screen);

    lv_obj_t * body = lv_obj_create(screen);
    lv_obj_remove_style_all(body);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(body, &style_panel, 0);
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_flex_grow(body, 1);
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(body, 0, 0);

    lv_obj_t * sidebar = lv_obj_create(body);
    lv_obj_remove_style_all(sidebar);
    lv_obj_remove_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_style(sidebar, &style_sidebar, 0);
    lv_obj_set_width(sidebar, layout_metrics.sidebar_width);
    lv_obj_set_height(sidebar, lv_pct(100));
    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * main = lv_obj_create(body);
    lv_obj_remove_style_all(main);
    lv_obj_add_style(main, &style_content, 0);
    lv_obj_set_flex_grow(main, 1);
    lv_obj_set_height(main, lv_pct(100));
    lv_obj_set_scroll_dir(main, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(main, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t * content = lv_obj_create(main);
    lv_obj_remove_style_all(content);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(content, layout_metrics.grid_width);
    lv_obj_set_height(content, LV_SIZE_CONTENT);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(content, layout_metrics.section_gap, 0);
    lv_obj_set_style_bg_color(content, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_align(content, LV_ALIGN_TOP_LEFT);
    content_root = content;

    lv_obj_t * section_targets[ARRAY_SIZE(sections)];
    for(size_t i = 0; i < ARRAY_SIZE(sections); ++i) {
        section_targets[i] = create_section(content, &sections[i]);
    }

    lv_obj_t * sidebar_title = lv_label_create(sidebar);
    lv_label_set_text(sidebar_title, "分类导航");
    lv_obj_add_style(sidebar_title, &style_muted, 0);

    create_sidebar_item(sidebar, "所有工具", content, true);
    create_sidebar_item(sidebar, "硬件/PCB", section_targets[0], false);
    create_sidebar_item(sidebar, "网络/串口调试", section_targets[1], false);
    create_sidebar_item(sidebar, "代码美化/转换", section_targets[2], false);
    create_sidebar_item(sidebar, "AI 助手", section_targets[3], false);
    create_sidebar_item(sidebar, "CRC/校验计算", section_targets[4], false);
    create_sidebar_item(sidebar, "进制/ASCII", section_targets[5], false);
    create_sidebar_item(sidebar, "时间/计算器", section_targets[6], false);
    create_sidebar_item(sidebar, "设计工具", section_targets[7], false);

    lv_obj_t * footer_hint = lv_label_create(content);
    lv_label_set_text(footer_hint, "更多工具开发中，欢迎提交建议...");
    lv_obj_add_style(footer_hint, &style_footer, 0);
    lv_obj_set_style_text_align(footer_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(footer_hint, lv_pct(100));

    lv_obj_t * footer = lv_obj_create(content);
    lv_obj_remove_style_all(footer);
    lv_obj_remove_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(footer, lv_pct(100));
    lv_obj_set_layout(footer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(footer, 24, 0);
    lv_obj_set_style_pad_ver(footer, 8, 0);

    create_text_node(footer, "粤ICP备2025393357号-2", &style_footer);
    create_text_node(footer, "粤公网安备44011502001545号", &style_footer);

    return screen;
}

lv_obj_t * home_page_get_content_root(void)
{
    return content_root;
}
