// src/MVVM/settings_vm.h
#ifndef SETTINGS_VM_H
#define SETTINGS_VM_H

#include "awtk.h"
#include "MVVM/core/emitter.h"

// settings_field_e / settings_lang_e / settings_unit_e / settings_display_e
// 全部在 model/settings_model.h 定义（model 层）；VM 层只 include 使用。
// settings_change_t 是 VM 层的事件 payload，定义在本头里。
#include "MVVM/model/settings_model.h"

BEGIN_C_DECLS

// 单 emitter 携带的事件载荷：field 标识变更字段，value 指向字段实际值。
typedef struct {
    settings_field_e field;
    const void*      value;
} settings_change_t;

ret_t settings_vm_init(void);
mvvm_emitter_t* settings_vm_get_emitter(void);   // 单一 emitter

// 统一 setter，取代旧的 5 个 per-field setter。
ret_t settings_vm_set(settings_field_e field, const void* value);

settings_lang_e    settings_vm_get_language(void);
settings_unit_e    settings_vm_get_unit(void);
uint8_t            settings_vm_get_brightness(void);
settings_display_e settings_vm_get_display(void);
bool_t             settings_vm_get_bluetooth(void);

const char* settings_vm_format_speed(int32_t kmh, settings_unit_e unit);
const char* settings_vm_format_distance(int32_t km, settings_unit_e unit);

END_C_DECLS

#endif
