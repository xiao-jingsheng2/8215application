// src/MVVM/core/binding.h
#ifndef MVVM_BINDING_H
#define MVVM_BINDING_H

#include "awtk.h"
#include "MVVM/core/emitter.h"

BEGIN_C_DECLS

typedef struct {
    mvvm_emitter_t* emitter;
    void* ctx;
} mvvm_binding_t;

static ret_t mvvm_binding_destroy(void* bind_ctx, event_t* e) {
    (void)e;
    mvvm_binding_t* b = (mvvm_binding_t*)bind_ctx;
    if (b && b->emitter) {
        mvvm_emitter_off(b->emitter, b->ctx);
    }
    TKMEM_FREE(b);
    return RET_OK;
}

END_C_DECLS

// 宏：订阅 emitter + 注册 EVT_DESTROY 自动注销
// 每次调用 TKMEM_ALLOC(sizeof(mvvm_binding_t))，view_init 时调用，非热路径
// 注意：宏参数不可命名为 emitter/ctx，否则会替换掉结构体成员名。
// MVVM_BIND(anchor, &dashboard_vm()->speed_em, on_speed_changed);
#define MVVM_BIND(_w, _em, _fn) do { \
    mvvm_emitter_on((_em), (_fn), (_w)); \
    mvvm_binding_t* _mvvm_b = TKMEM_ALLOC(sizeof(mvvm_binding_t)); \
    if (_mvvm_b != NULL) { \
        _mvvm_b->emitter = (_em); \
        _mvvm_b->ctx = (_w); \
        widget_on((_w), EVT_DESTROY, mvvm_binding_destroy, _mvvm_b); \
    } \
} while(0)

// 点击绑定：注册 EVT_CLICK handler。
// widget_on 由 widget 生命周期自动释放，无需 DESTROY 清理。
// 参数不可命名为 w/fn/ctx（保留命名空间），用 _w/_fn/_ctx。
#define MVVM_BIND_CLICK(_w, _fn, _ctx) do { \
    widget_on((_w), EVT_CLICK, (_fn), (_ctx)); \
} while(0)

#endif
