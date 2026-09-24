// src/model/settings_model.h
#ifndef SETTINGS_MODEL_H
#define SETTINGS_MODEL_H

#include "MVVM/core/emitter.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 数据领域枚举归属 model 层；VM 层通过 include model.h 即可拿到。
typedef enum {
    SETTINGS_LANG_ZH = 0,
    SETTINGS_LANG_EN,
    SETTINGS_LANG_MAX,
} settings_lang_e;

typedef enum {
    SETTINGS_UNIT_KM = 0,
    SETTINGS_UNIT_MILE,
    SETTINGS_UNIT_MAX,
} settings_unit_e;

typedef enum {
    SETTINGS_DISPLAY_DAY = 0,
    SETTINGS_DISPLAY_NIGHT,
    SETTINGS_DISPLAY_AUTO,
    SETTINGS_DISPLAY_MAX,
} settings_display_e;

// settings_field_e 正式定义在 model 层（Task 3）。
typedef enum {
    SETTINGS_FIELD_LANG = 0,
    SETTINGS_FIELD_UNIT,
    SETTINGS_FIELD_BRIGHTNESS,
    SETTINGS_FIELD_DISPLAY,
    SETTINGS_FIELD_BLUETOOTH,
    SETTINGS_FIELD_MAX,
} settings_field_e;

typedef struct {
    settings_lang_e    lang;
    settings_unit_e    unit;
    uint8_t            brightness;
    settings_display_e display;
    bool               bluetooth;
} settings_model_t;

int                     settings_model_init(void);
mvvm_emitter_t*         settings_model_get_emitter(void);
int                     settings_model_set(settings_field_e field,
                                           const void* value);
const settings_model_t* settings_model_get_data(void);

#ifdef __cplusplus
}
#endif
#endif