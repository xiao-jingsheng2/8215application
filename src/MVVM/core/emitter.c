// src/MVVM/core/emitter.c
#include "MVVM/core/emitter.h"
#include <stdlib.h>
#include <string.h>

void mvvm_emitter_init(mvvm_emitter_t* e, const char* name) {
    e->name = name;
    e->items = NULL;
    e->count = 0;
    e->iterating = false;
    e->remove_curr_iter = false;
    e->curr_iter = NULL;
}

static int mvvm_emitter_remove_node(mvvm_emitter_t* e,
                                    mvvm_emitter_item_t* target) {
    mvvm_emitter_item_t** pp = &e->items;
    while (*pp != NULL) {
        if (*pp == target) {
            mvvm_emitter_item_t* dead = *pp;
            *pp = dead->next;
            free(dead);
            e->count--;
            return MVVM_EMITTER_OK;
        }
        pp = &(*pp)->next;
    }
    return MVVM_EMITTER_NOT_FOUND;
}

int mvvm_emitter_on(mvvm_emitter_t* e,
                    void (*fn)(void*, const void*),
                    void* ctx) {
    if (e == NULL || fn == NULL) return MVVM_EMITTER_BAD_PARAMS;

    // 去重
    for (mvvm_emitter_item_t* it = e->items; it != NULL; it = it->next) {
        if (it->fn == fn && it->ctx == ctx) {
            return MVVM_EMITTER_OK;
        }
    }

    mvvm_emitter_item_t* item = (mvvm_emitter_item_t*)malloc(sizeof(mvvm_emitter_item_t));
    if (item == NULL) return MVVM_EMITTER_OOM;

    item->fn = fn;
    item->ctx = ctx;
    item->next = e->items;
    e->items = item;
    e->count++;
    return MVVM_EMITTER_OK;
}

int mvvm_emitter_off(mvvm_emitter_t* e, void* ctx) {
    if (e == NULL) return MVVM_EMITTER_BAD_PARAMS;

    mvvm_emitter_item_t* it = e->items;
    while (it != NULL) {
        if (it->ctx == ctx) {
            if (e->iterating && it == e->curr_iter) {
                e->remove_curr_iter = true;
                return MVVM_EMITTER_OK;
            }
            return mvvm_emitter_remove_node(e, it);
        }
        it = it->next;
    }
    return MVVM_EMITTER_NOT_FOUND;
}

void mvvm_emitter_emit(mvvm_emitter_t* e, const void* value) {
    if (e == NULL) return;
    e->iterating = true;
    mvvm_emitter_item_t* it = e->items;
    while (it != NULL) {
        mvvm_emitter_item_t* next = it->next;
        e->curr_iter = it;
        if (it->fn != NULL) {
            it->fn(it->ctx, value);
        }
        e->curr_iter = NULL;
        if (e->remove_curr_iter) {
            e->remove_curr_iter = false;
            mvvm_emitter_remove_node(e, it);
        }
        it = next;
    }
    e->iterating = false;
}

void mvvm_emitter_deinit(mvvm_emitter_t* e) {
    if (e == NULL) return;
    while (e->items != NULL) {
        mvvm_emitter_item_t* dead = e->items;
        e->items = dead->next;
        free(dead);
    }
    e->count = 0;
    e->items = NULL;
}

uint32_t mvvm_emitter_size(mvvm_emitter_t* e) {
    return e != NULL ? e->count : 0;
}
