# hcn/application/lvgl — lvgl 最小 UI 模块（迁移自 lvgl_home demo）

依据《DA_AC8215_lvgl程序流与代码迁移hcn统一编译梳理》方案 A 落地：
单一源树 + `HCN_APP_MODE` 开关（demo|full），代码管控于 hcn 三文件夹
（application / middleware / vehicle_services）。

## 模块构成（最小 UI 模块）

| 目录/文件 | 说明 |
|---|---|
| `main.c` | 进程入口：lv_init + GLFW/EGL 显示 + ui_init + 主循环（`lv_timer_handler` 5ms 协作轮询）；full 模式加 `vs_init()` + `dashboard_vm_flush()` |
| `lvgl_app/ui.c` | UI 装配：videofocusmanager 注册 Home 屏（最小集） |
| `lvgl_app/frameworks/` | `videofocusmanager`（屏幕导航栈）、`audiofocusmanager`、`app_common`、`audio_settings`、`bootproflog` |
| `lvgl_app/screens/ui_Home.c` | Home 屏（SquareLine 生成，bt/aa 调用已用 `HCN_APP_HAVE_BT/AA` 宏守卫） |
| `lvgl_app/screens/dashboard_view.c` | MVVM lvgl View（full 模式）：订阅 `dashboard_vm` emitter 刷新车速/转速/档位 label |
| `lvgl_app/components|fonts|images` | 组件/字体/图片资源（与原 lvgl_home 编译集一致） |
| `S01hcn_application` | init 脚本（迁自 S01lvgl_home） |

未迁入（保持原位，后续按需）：bt / androidauto / lvgl_carplay / rvc 适配层、
atcMusic/atcPicture/atcVideo 媒体源、atcpicture_decoder、WiFi 设置页。
原包 `source/packages/application/lvgl_home` 保留为 baseline。

## MVVM 与数据交互层（保留契约）

```
lib_mw vehicle_param 回调（CAN 线程）
  -> vehicle_services.c on_vehicle_change        [vs 门面，UI↔mw 唯一边界]
  -> dashboard_model_set()  emitter 置脏
  -> main 循环 dashboard_vm_flush()（GUI 线程 diff 派发）
  -> dashboard_view -> lvgl label 刷新
```

- `../src/MVVM/core/emitter.c`、`model/dashboard_model.c`：纯 C，UI 无关，原样复用
- `../src/MVVM/viewModel/dashboard_vm.c`：已去除 awtk.h 耦合（idle_queue → 置脏 +
  GUI 线程 flush），AWTK/lvgl 两种 View 共用同一实现
- application 不 include 任何 `<mw/xxx.h>`，只依赖 `vehicle_services.h` + MVVM

## 编译（buildroot 包 hcn-application）

```sh
# full 模式（默认，+ lib_mw + vehicle_services + MVVM）
make DEVICE=ac83xx VARIANT=userdebug hcn-application-rebuild

# 切 demo 模式：menuconfig 中 HCN application build mode -> demo
#   BR2_PACKAGE_HCN_APPLICATION=y + BR2_PACKAGE_HCN_APPLICATION_DEMO
# 强制重编（含 vehicle_services）
make DEVICE=ac83xx VARIANT=userdebug hcn-application-dirclean
make DEVICE=ac83xx VARIANT=userdebug hcn-application-rebuild
```

CMake 选项：`-DHCN_APP_MODE=demo|full`（由 Config.in choice 映射）。

full 链接：`vehicle_services mw rt`（vehicle_services 在 lib_mw 前；cJSON 需 -lm）。

## buildroot 构建路径注入（重要）

buildroot local 站点包把 `application/lvgl` rsync 到 `out/build/ac83xx/<pkg>/`
构建，拷贝目录中源码树相对路径（`../../src`、`../../../../../packages`）不可
解析。`hcn-application.mk` 因此注入源码树绝对路径：

- `-DHCN_APP_APPSRC=source/vendor/hcn/application/src`（MVVM 头与源码）
- `-DHCN_APP_SETTINGS_INC=source/packages/graphics/libsettings_atc/include`
  （audio_settings.c 所需 AtcAudioSettings.h）

在源码树内直接跑 cmake 时使用 CMake 内默认相对路径，无需传参。
