#include "ui_font.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if LV_USE_FREETYPE
#include "src/libs/freetype/lv_freetype.h"
#endif

#define UI_FONT_CACHE_MAX 32
#define UI_FONT_DEFAULT_PATH "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
#define UI_ICON_FONT_FILE "MaterialSymbolsOutlined.ttf"

typedef struct {
    int size;
    lv_font_t * font;
} ui_font_cache_entry_t;

static ui_font_cache_entry_t s_cache[UI_FONT_CACHE_MAX];
static int s_cache_count = 0;

/* Separate cache for the Material Symbols icon font (independent of body text). */
static ui_font_cache_entry_t s_icon_cache[UI_FONT_CACHE_MAX];
static int s_icon_cache_count = 0;

static const char * ui_font_path(void)
{
    const char * env = getenv("UI_FONT_PATH");
    if(env != NULL && env[0] != '\0') {
        return env;
    }
    return UI_FONT_DEFAULT_PATH;
}

const lv_font_t * ui_font_get(int size)
{
    int i;
    for(i = 0; i < s_cache_count; i++) {
        if(s_cache[i].size == size) {
            return s_cache[i].font;
        }
    }

#if LV_USE_FREETYPE
    lv_font_t * ft = lv_freetype_font_create(ui_font_path(),
                                              LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                              (uint32_t)size,
                                              LV_FREETYPE_FONT_STYLE_NORMAL);
    if(ft != NULL) {
        if(s_cache_count >= UI_FONT_CACHE_MAX) {
            /* Evict oldest entry */
            lv_freetype_font_delete(s_cache[0].font);
            memmove(&s_cache[0], &s_cache[1],
                    (UI_FONT_CACHE_MAX - 1) * sizeof(s_cache[0]));
            s_cache_count = UI_FONT_CACHE_MAX - 1;
        }
        s_cache[s_cache_count].size = size;
        s_cache[s_cache_count].font = ft;
        s_cache_count++;
        return ft;
    }
#endif

    /* Fallback to built-in fonts */
    return size <= 18 ? &lv_font_montserrat_14 : &lv_font_montserrat_24;
}

/* ---- Icon font (Material Symbols) ------------------------------------- */

static const char * ui_icon_font_path_resolved(void)
{
    const char * env = getenv("UI_ICON_FONT_PATH");
    if(env != NULL && env[0] != '\0' && access(env, R_OK) == 0) {
        return env;
    }
    /* Probe candidate relative paths so the icon font is found regardless of
     * whether the runtime CWD is the task build dir, runtime_project/, or the
     * repository root. */
    static char buf[512];
    const char * candidates[] = {
        "assets/" UI_ICON_FONT_FILE,
        "../assets/" UI_ICON_FONT_FILE,
        "../../runtime_project/assets/" UI_ICON_FONT_FILE,
        "runtime_project/assets/" UI_ICON_FONT_FILE,
    };
    size_t i;
    for(i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        if(access(candidates[i], R_OK) == 0) {
            strncpy(buf, candidates[i], sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';
            return buf;
        }
    }
    return NULL;
}

const lv_font_t * ui_icon_font_get(int size)
{
    int i;
    for(i = 0; i < s_icon_cache_count; i++) {
        if(s_icon_cache[i].size == size) {
            return s_icon_cache[i].font;
        }
    }

#if LV_USE_FREETYPE
    const char * path = ui_icon_font_path_resolved();
    if(path != NULL) {
        lv_font_t * ft = lv_freetype_font_create(path,
                                                  LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
                                                  (uint32_t)size,
                                                  LV_FREETYPE_FONT_STYLE_NORMAL);
        if(ft != NULL) {
            if(s_icon_cache_count >= UI_FONT_CACHE_MAX) {
                lv_freetype_font_delete(s_icon_cache[0].font);
                memmove(&s_icon_cache[0], &s_icon_cache[1],
                        (UI_FONT_CACHE_MAX - 1) * sizeof(s_icon_cache[0]));
                s_icon_cache_count = UI_FONT_CACHE_MAX - 1;
            }
            s_icon_cache[s_icon_cache_count].size = size;
            s_icon_cache[s_icon_cache_count].font = ft;
            s_icon_cache_count++;
            return ft;
        }
    }
#endif

    /* Fallback to built-in fonts (they carry the LV_SYMBOL_* icon subset). */
    return size <= 18 ? &lv_font_montserrat_14 : &lv_font_montserrat_24;
}

void ui_font_cleanup(void)
{
#if LV_USE_FREETYPE
    int i;
    for(i = 0; i < s_cache_count; i++) {
        lv_freetype_font_delete(s_cache[i].font);
        s_cache[i].font = NULL;
    }
    for(i = 0; i < s_icon_cache_count; i++) {
        lv_freetype_font_delete(s_icon_cache[i].font);
        s_icon_cache[i].font = NULL;
    }
#endif
    s_cache_count = 0;
    s_icon_cache_count = 0;
}
