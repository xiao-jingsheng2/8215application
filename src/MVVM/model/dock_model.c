// src/model/dock_model.c
#include "MVVM/model/dock_model.h"

static dock_model_t   g_state = { .current_tab = DOCK_TAB_INFO };
static mvvm_emitter_t g_em;
static bool           g_is_init = false;

int dock_model_init(void) {
    if (g_is_init) return 0;
    mvvm_emitter_init(&g_em, "dock.model");
    g_is_init = true;
    return 0;
}

mvvm_emitter_t* dock_model_get_emitter(void) { return &g_em; }

const dock_model_t* dock_model_get_data(void) { return &g_state; }

int dock_model_set_current_tab(dock_tab_e tab) {
    if (tab >= DOCK_TAB_MAX) return -1;
    if (tab == g_state.current_tab) return 0;
    g_state.current_tab = tab;
    mvvm_emitter_emit(&g_em, &g_state);
    return 0;
}
