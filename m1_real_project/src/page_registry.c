#include "page_registry.h"

#include <stddef.h>
#include <string.h>

#include "generated_page_registry.h"
#include "home_page.h"
#include "token_page.h"

static const m1_page_descriptor_t g_pages[] = {
    {
        .id = "home",
        .name = "Home Page",
        .create = home_page_create,
        .get_content_root = home_page_get_content_root,
    },
    {
        .id = "token",
        .name = "Token Landing Page",
        .create = token_page_create,
        .get_content_root = token_page_get_content_root,
    },
};

static m1_page_descriptor_t g_combined_pages[256];

static size_t fill_combined_pages(void)
{
    const m1_page_descriptor_t * generated_pages = m1_generated_page_list();
    size_t base_count = sizeof(g_pages) / sizeof(g_pages[0]);
    size_t generated_count = m1_generated_page_count();
    size_t total = base_count + generated_count;
    size_t i;

    if(total > sizeof(g_combined_pages) / sizeof(g_combined_pages[0])) {
        total = sizeof(g_combined_pages) / sizeof(g_combined_pages[0]);
        LV_LOG_WARN("Page list truncated to %zu entries", total);
    }

    for(i = 0; i < base_count && i < total; ++i) {
        g_combined_pages[i] = g_pages[i];
    }

    for(i = 0; i < generated_count && (base_count + i) < total; ++i) {
        g_combined_pages[base_count + i] = generated_pages[i];
    }

    return total;
}

const m1_page_descriptor_t * m1_page_default(void)
{
    return &g_pages[0];
}

const m1_page_descriptor_t * m1_page_find(const char * id)
{
    const m1_page_descriptor_t * pages = NULL;
    size_t count = 0;
    size_t i;

    if(id == NULL || id[0] == '\0') {
        return m1_page_default();
    }

    pages = m1_page_list(&count);
    for(i = 0; i < count; ++i) {
        if(strcmp(pages[i].id, id) == 0) {
            return &pages[i];
        }
    }

    return NULL;
}

const m1_page_descriptor_t * m1_page_list(size_t * count)
{
    size_t total = fill_combined_pages();

    if(count != NULL) {
        *count = total;
    }

    return g_combined_pages;
}
