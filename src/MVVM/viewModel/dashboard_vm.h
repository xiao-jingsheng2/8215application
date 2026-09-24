// src/MVVM/dashboard_vm.h
//
// UI 无关化适配（迁移文档第 6 节：AWTK/lvgl View 共用单一 VM 实现）：
//  - 去除 awtk.h 耦合：ret_t/bool_t/idle_queue -> 标准C + 置脏标志
//  - 派发契约不变：vm emitter 回调始终在 GUI 线程执行——
//      AWTK 侧：idle 中调 dashboard_vm_flush()（原 idle_queue 等价）
//      lvgl 侧：main 循环每帧调 dashboard_vm_flush()
//  - 返回码：0=OK（与 awtk RET_OK 数值语义对齐），负=错误
#ifndef DASHBOARD_VM_H
#define DASHBOARD_VM_H

#include "MVVM/core/emitter.h"
#include "MVVM/model/dashboard_model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    dashboard_field_e field;
    const void*       value;
} dashboard_vm_change_t;

int               dashboard_vm_init(void);
mvvm_emitter_t*   dashboard_vm_emitter(void);

/* GUI 线程冲刷:全量 diff model 与上次已推送快照,有变化再 emit。
 * model 变更(CAN 线程)只置脏;本函数由 GUI 线程周期调用消费脏标志。 */
void              dashboard_vm_flush(void);

#ifdef __cplusplus
}
#endif
#endif
