// src/MVVM/dock_vm.h
#ifndef DOCK_VM_H
#define DOCK_VM_H

#include "awtk.h"
#include "MVVM/core/emitter.h"
#include "MVVM/model/dock_model.h"   /* dock_tab_e 由 model 层定义 */

#ifdef __cplusplus
extern "C" {
#endif

ret_t dock_vm_init(void);
mvvm_emitter_t* dock_vm_get_emitter(void);
ret_t dock_vm_set_current_tab(dock_tab_e tab);
dock_tab_e dock_vm_get_current_tab(void);

#ifdef __cplusplus
}
#endif
#endif