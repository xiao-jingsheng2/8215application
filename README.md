# hcn/application — HCN 应用层（lvgl 主线）

AWTK 时代代码已于 2026-09 清退（原 SConstruct/SCons、3rd 组件库、res 资产、
Windows 模拟器产物等已删除，历史版本可经本仓库 git 找回）。当前结构：

```
application/
├── lvgl/            # lvgl 最小 UI 模块（主线，buildroot cmake 包 hcn-application）
│   ├── main.c       #   进程入口：lv_init + GLFW/EGL + ui_init + 5ms 协作轮询
│   ├── lvgl_app/    #   ui.c 装配 + Home 屏 + frameworks + dashboard_view(MVVM View)
│   ├── CMakeLists.txt  # HCN_APP_MODE=demo|full 开关
│   └── S01hcn_application
├── src/
│   ├── MVVM/        # UI 无关数据层：core/emitter + model/dashboard_model（纯 C，
│   │                # 被 lvgl full 模式编译链入）+ dashboard_vm（已去 AWTK 耦合）
│   │                # dock/settings/time VM 为 AWTK 遗留参考文件，未参与编译
│   └── logic/       # 业务逻辑遗留参考（依赖已删除的 proxy/view 等，未参与编译，
│                    # 迁移两轮逻辑时按需提取）
└── 编译使用.md      # 编译操作手册
```

- 编译规则与 MVVM 数据通路详见 `lvgl/README.md`
- 依赖链：`lib_mw → vehicle_services → hcn-application`（buildroot 自动排序）
- AWTK 历史流程说明见 `编译使用.md` 末节（仅留档）
