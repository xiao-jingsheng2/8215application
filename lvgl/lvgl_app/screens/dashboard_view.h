// dashboard_view.h — MVVM lvgl View（HCN_APP_FULL 模式）
//
// 车辆数据面板：仿 AWTK 侧 view/home_view/speed_view.c 的绑定范式，
// View 订阅 dashboard_vm emitter（GUI 线程派发），按 field 刷新 lvgl 控件。
// 数据流（迁移/融合文档契约）：
//   lib_mw vehicle_param 回调 -> vs on_vehicle_change -> dashboard_model_set
//   -> emitter -> dashboard_vm(置脏, GUI 线程 flush diff) -> 本 View -> lvgl
#ifndef DASHBOARD_VIEW_H
#define DASHBOARD_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

/** 在 parent（Home 屏）上创建车辆数据面板并订阅 dashboard_vm。幂等。 */
void dashboard_view_attach(lv_obj_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* DASHBOARD_VIEW_H */
