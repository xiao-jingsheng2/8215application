# =====================================================================
#  hcn_application — HCN lvgl 应用（迁移自 lvgl_home demo 最小 UI 模块）
#
#  依据《DA_AC8215_lvgl程序流与代码迁移hcn统一编译梳理》方案 A：
#   - 单一源树 + HCN_APP_MODE 模式开关（demo|full）
#   - demo：纯 lvgl UI（videofocusmanager 导航 + Home 屏），无中间件
#   - full ：+ vehicle_services(C 门面) + lib_mw(中间件) + MVVM(dashboard)
#   - SDK 结构零改动：lvgl/显示/工具链库均来自 buildroot staging
#
#  构建入口：buildroot cmake 包 hcn-application
#    make DEVICE=ac83xx VARIANT=userdebug hcn-application-rebuild
# =====================================================================
cmake_minimum_required(VERSION 3.8.2)
project(hcn_application C CXX)

option(HCN_APP_MODE "demo|full" "full")

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "HCN_APP_MODE=${HCN_APP_MODE}")
message(STATUS "AC83XX_BOOT_DEVICE=${AC83XX_BOOT_DEVICE}")

add_subdirectory(lvgl_app)

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-s")

target_include_directories(lvgl_app PUBLIC ${PROJECT_SOURCE_DIR}/lvgl_app)
target_compile_definitions(lvgl_app PUBLIC
        LV_CONF_INCLUDE_SIMPLE
        LV_FONT_SOURCE_HAN_SANS_SC_16_CJK=1)

if(HCN_APP_MODE STREQUAL "full")
    target_compile_definitions(lvgl_app PUBLIC HCN_APP_FULL)
endif()

if(TARGET lvgl)
    target_compile_definitions(lvgl PUBLIC
            LV_CONF_INCLUDE_SIMPLE
            LV_FONT_SOURCE_HAN_SANS_SC_16_CJK=1)
    target_include_directories(lvgl PUBLIC ${PROJECT_SOURCE_DIR})
endif()

# application/src（MVVM "MVVM/..." 头前缀）：默认适配源码树内构建；
# buildroot 拷贝构建时由 hcn-application.mk 注入绝对路径（同 lvgl_app 侧说明）
set(HCN_APP_APPSRC "${PROJECT_SOURCE_DIR}/../src" CACHE PATH
    "hcn application src dir (MVVM), injected by hcn-application.mk for buildroot builds")

target_include_directories(lvgl_app PUBLIC ${HCN_APP_APPSRC})

add_executable(hcn_application
        ${PROJECT_SOURCE_DIR}/main.c
        ${PROJECT_SOURCE_DIR}/mouse_cursor_icon.c)

target_compile_definitions(hcn_application PRIVATE LV_CONF_INCLUDE_SIMPLE)
target_include_directories(hcn_application PRIVATE ${HCN_APP_APPSRC})

if(HCN_APP_MODE STREQUAL "full")
    target_compile_definitions(hcn_application PRIVATE HCN_APP_FULL)
endif()

# jpeg/png/image：liblvgl.a 自带解码器依赖（lv_libjpeg_turbo/lv_libpng/lv_atcimage
# 的 imgdec_*），与 cluster/lvgl_demo 链接清单一致 —— 不只是媒体应用需要
target_link_libraries(hcn_application
    lvgl lvgl_app
    m pthread glfw GLESv2 EGL settings_atc
    jpeg png image)

if(HCN_APP_MODE STREQUAL "full")
    # 中间层链入（迁移文档 9.1）：vehicle_services 依赖 lib_mw 符号，
    # 故 vehicle_services 在前；cJSON(sqrt/fabs) 需 -lm；OSAL 需 -lrt。
    # lib_mw 用 -l:lib_mw.a 精确文件名链接：staging 产物是 lib_mw.a（带下划线），
    # 写 mw 会展开成 -lmw 去找 libmw.so/libmw.a，ld 报 cannot find -lmw。
    target_link_libraries(hcn_application
        vehicle_services -l:lib_mw.a rt)
endif()

set_target_properties(hcn_application PROPERTIES LINKER_LANGUAGE CXX)
install(TARGETS hcn_application DESTINATION bin)
install(PROGRAMS  ${PROJECT_SOURCE_DIR}/S01hcn_application DESTINATION /etc/init.d/)
