// src/MVVM/core/emitter.h
// 纯 C 事件订阅器：不依赖 awtk。mvvm 模型层与 VM 层共享。
#ifndef MVVM_EMITTER_H
#define MVVM_EMITTER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 返回码：与旧 awtk ret_t 数值语义对齐（0=OK，负=错误） */
#define MVVM_EMITTER_OK        0
#define MVVM_EMITTER_FAIL     -1
#define MVVM_EMITTER_NOT_FOUND -2
#define MVVM_EMITTER_OOM       -3
#define MVVM_EMITTER_BAD_PARAMS -4

struct _mvvm_emitter_item_t;
typedef struct _mvvm_emitter_item_t mvvm_emitter_item_t;

typedef struct _mvvm_emitter_item_t {
    void (*fn)(void* ctx, const void* value);
    void* ctx;
    struct _mvvm_emitter_item_t* next;
} mvvm_emitter_item_t;

typedef struct {
    const char* name;
    mvvm_emitter_item_t* items;
    uint32_t count;
    bool iterating;
    bool remove_curr_iter;
    mvvm_emitter_item_t* curr_iter;
} mvvm_emitter_t;

void    mvvm_emitter_init(mvvm_emitter_t* e, const char* name);
int     mvvm_emitter_on(mvvm_emitter_t* e,
                        void (*fn)(void*, const void*),
                        void* ctx);
int     mvvm_emitter_off(mvvm_emitter_t* e, void* ctx);
void    mvvm_emitter_emit(mvvm_emitter_t* e, const void* value);
void    mvvm_emitter_deinit(mvvm_emitter_t* e);
uint32_t mvvm_emitter_size(mvvm_emitter_t* e);

#ifdef __cplusplus
}
#endif

#endif
