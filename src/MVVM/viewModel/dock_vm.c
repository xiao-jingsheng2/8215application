// src/MVVM/dock_vm.c
#include "dock_vm.h"
#include "MVVM/model/dock_model.h"

static mvvm_emitter_t g_emitter;
static dock_tab_e     g_last_seen = DOCK_TAB_MAX;  // 哨兵：首 emit 必命中
static bool_t         g_is_init   = FALSE;

// payload 指针传 model 内稳定地址 &cur->current_tab，
// 避免依赖栈变量生命周期（即便将来 emit 改成异步也安全）。
static void emit_change(const dock_tab_e* tab_ptr) {
    mvvm_emitter_emit(&g_emitter, tab_ptr);
}

static void on_dock_model_changed(void* ctx, const void* value) {
    (void)ctx;
    if (value == NULL) return;
    const dock_model_t* cur = (const dock_model_t*)value;
    if (cur->current_tab == g_last_seen) return;
    g_last_seen = cur->current_tab;
    emit_change(&cur->current_tab);
}

ret_t dock_vm_init(void) {
    if (g_is_init) return RET_OK;
    mvvm_emitter_init(&g_emitter, "dock.vm");
    dock_model_init();
    g_last_seen = dock_model_get_data()->current_tab;
    mvvm_emitter_on(dock_model_get_emitter(), on_dock_model_changed, NULL);
    g_is_init = TRUE;
    return RET_OK;
}

mvvm_emitter_t* dock_vm_get_emitter(void) { return &g_emitter; }

ret_t dock_vm_set_current_tab(dock_tab_e tab) {
    return dock_model_set_current_tab(tab);
}

dock_tab_e dock_vm_get_current_tab(void) {
    return dock_model_get_data()->current_tab;
}