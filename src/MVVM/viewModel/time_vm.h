// src/MVVM/time_vm.h
#ifndef TIME_VM_H
#define TIME_VM_H

#include "awtk.h"
#include "MVVM/core/emitter.h"

BEGIN_C_DECLS

typedef struct {
    int32_t hour;
    int32_t minute;
    bool_t colon_visible;
} time_data_t;

ret_t time_vm_init(void);

mvvm_emitter_t* time_vm_get_time_emitter(void);

ret_t time_vm_set_time(int32_t hour, int32_t minute);

void time_vm_get_current(int32_t* hour, int32_t* minute);
void time_vm_poll_once(void);

END_C_DECLS

#endif