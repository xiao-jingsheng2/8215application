// src/model/dock_model.h
#ifndef DOCK_MODEL_H
#define DOCK_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include "MVVM/core/emitter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DOCK_TAB_INFO = 0,
    DOCK_TAB_NAVI,
    DOCK_TAB_MUSIC,
    DOCK_TAB_PHONE,
    DOCK_TAB_SETTING,
    DOCK_TAB_MAX,
} dock_tab_e;

typedef struct {
    dock_tab_e current_tab;
} dock_model_t;

int                     dock_model_init(void);
mvvm_emitter_t*         dock_model_get_emitter(void);
int                     dock_model_set_current_tab(dock_tab_e tab);
const dock_model_t*     dock_model_get_data(void);

#ifdef __cplusplus
}
#endif

#endif
