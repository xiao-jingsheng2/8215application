// src/MVVM/dashboard_vm.c

#include "dashboard_vm.h"
#include <string.h>
#include <stdbool.h>

static bool             g_t_init = false;
/* model 变更置脏标志：写者线程(vehicle_services/CAN 上下文)只置位，
 * GUI 线程在 dashboard_vm_flush() 中消费——emitter 派发始终在 GUI 线程，
 * 保证 lvgl/awtk 控件操作线程安全（替代原 awtk idle_queue 范式）。 */
static volatile bool    g_pending = false;
static mvvm_emitter_t   g_emitter;
static dashboard_model_t  g_sent;

static void emit_field(dashboard_field_e field, const void* value) {
    dashboard_vm_change_t ev;
    ev.field = field;
    ev.value = value;
    mvvm_emitter_emit(&g_emitter, &ev);
}


static void vm_flush_locked(void) {
    const dashboard_model_t* cur = dashboard_model_get_data();
    if (cur == NULL) return;

    if (cur->speed != g_sent.speed) {
        g_sent.speed = cur->speed;
        emit_field(DASHBOARD_FIELD_SPEED, &cur->speed);
    }
    if (cur->rpm != g_sent.rpm) {
        g_sent.rpm = cur->rpm;
        emit_field(DASHBOARD_FIELD_RPM, &cur->rpm);
    }
    if (cur->gear != g_sent.gear) {
        g_sent.gear = cur->gear;
        emit_field(DASHBOARD_FIELD_GEAR, &cur->gear);
    }
    if (cur->power != g_sent.power) {
        g_sent.power = cur->power;
        emit_field(DASHBOARD_FIELD_POWER, &cur->power);
    }
    if (cur->battery != g_sent.battery) {
        g_sent.battery = cur->battery;
        emit_field(DASHBOARD_FIELD_BATTERY, &cur->battery);
    }
    if (memcmp(cur->signals, g_sent.signals, sizeof(cur->signals)) != 0) {
        memcpy(g_sent.signals, cur->signals, sizeof(cur->signals));
        emit_field(DASHBOARD_FIELD_SIGNALS, cur->signals);
    }
}


/* model emitter 回调：可能在中间件写线程(CAN/vehicle_param)上下文执行，
 * 只置脏标志，不 emit、不做任何 UI 操作。 */
static void on_model_changed(void* ctx, const void* value) {
    (void)ctx;
    (void)value;
    g_pending = true;
}

int dashboard_vm_init(void) {
    if (g_t_init) return 0;

    mvvm_emitter_init(&g_emitter, "dashboard.vm");

    memset(&g_sent, 0, sizeof(g_sent));
    g_sent.gear = DASHBOARD_GEAR_INVALID;

    dashboard_model_init();
    mvvm_emitter_on(dashboard_model_get_emitter(),
                    on_model_changed, NULL);

    g_t_init = true;

    // 初始冲刷:把当前 model 值同步给 UI(若 model 尚无数据则为零值,无副作用)。
    // 置脏而非直接 flush：此时 View 多半尚未订阅，首次 GUI 线程 flush 生效。
    g_pending = true;

    return 0;
}

// 供 GUI 线程主动冲刷(如页面初始化后、进入下一页重新显示时)。
// lvgl: main 循环每帧调用；AWTK: idle 中调用（与 idle 链等效）。
void dashboard_vm_flush(void) {
    if (!g_t_init || !g_pending) return;
    g_pending = false;
    vm_flush_locked();
}

mvvm_emitter_t* dashboard_vm_emitter(void) {
    return &g_emitter;
}
