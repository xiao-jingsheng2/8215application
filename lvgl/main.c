
/**
 * @file main
 *
 * HCN lvgl application entry — migrated from lvgl_home demo (minimal UI module)
 * per "DA_AC8215_lvgl程序流与代码迁移hcn统一编译梳理" doc:
 *   - program flow: lv_init + GLFW/EGL display -> ui_init -> main loop
 *     (lv_timer_handler, 5ms cooperative poll), single thread
 *   - HCN_APP_MODE=full: bootstrap vehicle_services (vs_init) and dispatch
 *     MVVM dashboard_vm pending changes in the main loop
 *   - HCN_APP_MODE=demo: pure lvgl UI baseline, no middleware
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
#include "lvgl/lvgl.h"
#include "ui.h"
#include "lvgl_app/frameworks/audio_settings.h"
#include "lvgl_app/frameworks/bootproflog.h"
#include <lvgl/src/drivers/evdev/lv_evdev.h>

#ifdef HCN_APP_FULL
/* full 模式：vehicle_services C 门面 + MVVM ViewModel（UI↔中间件数据交互层） */
#include "vehicle_services.h"
#include "MVVM/viewModel/dashboard_vm.h"
#endif

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hcn_app_log_print(lv_log_level_t level, const char *buf);

/**********************
 *  VARIABLES
 **********************/
static lv_display_t *s_display = NULL;
extern lv_img_dsc_t mouse_cursor_icon;

static void hcn_app_log_print(lv_log_level_t level, const char *buf)
{
  (void)level;

  if(buf != NULL) {
    syslog(LOG_INFO, "%s", buf);
  }
}

static void hcn_app_set_mouse_cursor(lv_indev_t *indev)
{
  if(indev == NULL) {
    return;
  }

  lv_obj_t * cursor_obj = lv_image_create(lv_layer_top());
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);
  lv_indev_set_cursor(indev, cursor_obj);
}

static void hcn_app_evdev_discovered(lv_indev_t *indev, lv_evdev_type_t type, void *user_data)
{
  LV_UNUSED(user_data);

  if(indev == NULL) {
    return;
  }

  if(s_display != NULL) {
    lv_indev_set_display(indev, s_display);
  }

  if(type == LV_EVDEV_TYPE_REL) {
    syslog(LOG_INFO, "[hcn-app] mouse device discovered");
    hcn_app_set_mouse_cursor(indev);
  } else if(type == LV_EVDEV_TYPE_ABS) {
    syslog(LOG_INFO, "[hcn-app] touch device discovered");
  }
}

static void getScreenSize(int *x , int *y) {

    int fb = open("/dev/fb0", O_RDONLY);
    if (fb < 0) {
        syslog(LOG_INFO, "[hcn-app] fb = open(/dev/fb0, O_RDONLY) failed");
        return;
    }
    struct fb_var_screeninfo vinfo;
    if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        syslog(LOG_INFO, "[hcn-app] ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) failed");
        close(fb);
        return;
    }

    syslog(LOG_INFO, "[hcn-app] getScreenSize Resolution width = %u  height = %u ", vinfo.xres, vinfo.yres);
    syslog(LOG_INFO, "[hcn-app] getScreenSize Virtual Resolution width = %u  height = %u ", vinfo.xres_virtual, vinfo.yres_virtual);
    *x = vinfo.xres;
    *y = vinfo.yres;

    close(fb);
}

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  writeBootProf("hcn_application main enter");
  openlog("hcn_application", LOG_PID, LOG_USER);
  writeBootProf("hcn_application before lv_init");
  lv_init();
  lv_log_register_print_cb(hcn_app_log_print);

#if 1
    writeBootProf("hcn_application before lv_glfw_window_create");
    /* create a window and initialize OpenGL */

    int width = 0 ;
    int height = 0;
    getScreenSize(&width, &height);

    lv_glfw_window_t * window = lv_glfw_window_create(width, height, false);

    /* create a display that flushes to a texture */
    lv_display_t * texture = lv_opengles_texture_create(width, height);
    lv_display_set_default(texture);
    s_display = texture;

    /* add the texture to the window */
    unsigned int texture_id = lv_opengles_texture_get_texture_id(texture);
    lv_glfw_texture_t * window_texture = lv_glfw_window_add_texture(window, texture_id, width, height);

    writeBootProf("hcn_application before lv_glfw_texture_get_mouse_indev");
    /* get the mouse indev of the window texture */
    lv_indev_t * mouse = lv_glfw_texture_get_mouse_indev(window_texture);
    if(mouse != NULL) {
      lv_indev_set_display(mouse, s_display);
      hcn_app_set_mouse_cursor(mouse);
    }
#else
    lv_display_t *display = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(display, "/dev/fb0");
    s_display = display;
#endif

#ifdef HCN_APP_FULL
  /* 引导中间件：mw_init + 注册 vehicle_param 变更回调 -> dashboard_model。
   * 置于 ui_init 之前，保证 UI 订阅前数据通路就绪（vs_init 幂等）。 */
  writeBootProf("hcn_application before vs_init");
  if (vs_init() != 0) {
      syslog(LOG_ERR, "[hcn-app] vehicle services init failed, continue in UI-only mode");
  }
#endif

  writeBootProf("hcn_application before ui_init");

  ui_init();
  lv_evdev_discovery_start(hcn_app_evdev_discovered, NULL);

  writeBootProf("hcn_application done");
  audio_settings_init();
  writeBootProf("audio_settings_init done");

  while(1) {
    /* Periodically call the lv_task handler.
     * It could be done in a timer interrupt or an OS task too.*/
    lv_timer_handler();
#ifdef HCN_APP_FULL
    /* MVVM 派发：将 dashboard_model 变更(CAN 线程写入置脏)冲刷到 lvgl View。
     * 与 AWTK idle 等价：保证 emitter 派发始终在 GUI 线程(本循环)执行。 */
    dashboard_vm_flush();
#endif
    usleep(5 * 1000);
  }

#ifdef HCN_APP_FULL
  vs_deinit();
#endif
  closelog();

  return 0;
}
