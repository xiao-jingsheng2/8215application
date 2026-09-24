// src/model/dashboard_model.h
// 纯 C 自包含数据模型：不依赖 awtk，也不依赖 application 内部代理头。
// 数据来源由外部（vehicle_services 等）通过 dashboard_model_set() 写入；
// 本模型不自轮询、不自拉取。
#ifndef DASHBOARD_MODEL_H
#define DASHBOARD_MODEL_H

#include <stdbool.h>
#include <stdint.h>
#include "MVVM/core/emitter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DASHBOARD_SIGNAL_MAX 12

typedef enum {
    DASHBOARD_GEAR_PARK = 0,
    DASHBOARD_GEAR_NEUTRAL,
    DASHBOARD_GEAR_DRIVE,
    DASHBOARD_GEAR_REVERSE,
    DASHBOARD_GEAR_INVALID,
} dashboard_gear_e;

// 数据领域枚举归属 model 层；VM 层通过 include model.h 即可拿到。
typedef enum {
    DASHBOARD_FIELD_SPEED = 0,
    DASHBOARD_FIELD_RPM,
    DASHBOARD_FIELD_GEAR,
    DASHBOARD_FIELD_POWER,
    DASHBOARD_FIELD_BATTERY,
    DASHBOARD_FIELD_SIGNALS,
    DASHBOARD_FIELD_MAX,
} dashboard_field_e;

typedef struct {
    int32_t          speed;
    int32_t          rpm;
    dashboard_gear_e gear;        // 变速档位，取值 DASHBOARD_GEAR_* 枚举
    int32_t          power;
    int32_t          battery;
    bool             signals[DASHBOARD_SIGNAL_MAX];
} dashboard_model_t;

int                     dashboard_model_init(void);
void                    dashboard_model_deinit(void);

mvvm_emitter_t*         dashboard_model_get_emitter(void);
const dashboard_model_t* dashboard_model_get_data(void);
int                     dashboard_model_set(dashboard_field_e field,
                                            const void* value);

#ifdef __cplusplus
}
#endif
#endif
