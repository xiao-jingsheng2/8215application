/*
 * dvr_page.c — DVR page init / destroy (LVGL 9.x port)
 *
 * Ported from AWTK dvr_page.c (dc002-0617 branch, AMT630HV100).
 *
 * Target: lvgl/lvgl_app/screens/dvr_page.c
 * Source: HCN_DC001/src/pages/dvr_page.c
 *
 * On entry: creates a screen, initializes DVR view (MAIN state),
 *           starts preview, queries DVR status, pre-fetches version/TF.
 * On exit:  stops preview, restores sensor switch.
 */

#include "../ui.h"
#include "dvr_view.h"
#include "dvr_api.h"

#define DVR_PREVIEW_MODE_FRONT_REAR  2

static lv_obj_t *dvr_screen = NULL;

static void on_dvr_screen_delete(lv_event_t *e)
{
    (void)e;
    printf("DVR-EXIT: on_dvr_screen_delete enter, preview_enable=%d\n",
           dvr_api_get_preview_enable());

    dvr_view_destroy();

    if (dvr_api_get_preview_enable()) {
        dvr_stop_preview();
    }
    dvr_set_sensor_switch_enable(1);
    dvr_screen = NULL;

    printf("DVR-EXIT: on_dvr_screen_delete done\n");
}

/*
 * dvr_page_create — Create and show the DVR screen.
 *
 * Called by the page navigator when entering DVR mode.
 * Returns the created screen object.
 */
lv_obj_t *dvr_page_create(void)
{
    if (dvr_screen) {
        printf("DVR: dvr_page_create — screen already exists, loading\n");
        lv_screen_load(dvr_screen);
        return dvr_screen;
    }

    dvr_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(dvr_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dvr_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(dvr_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Register screen delete callback (equivalent of EVT_WINDOW_CLOSE) */
    lv_obj_add_event_cb(dvr_screen, on_dvr_screen_delete, LV_EVENT_DELETE, NULL);

    /* Build the DVR UI */
    dvr_view_init(dvr_screen);

    /* Display window: full-width (0,0,1024,500) */
    dvr_api_set_display_window(0, 0, 1024, 500);

    /* Start preview on page entry */
    dvr_api_set_preview_enable(1);

    /* Lock preview to front+rear dual-camera mode */
    dvr_api_view_switch(DVR_PREVIEW_MODE_FRONT_REAR);

    /* Query DVR status (SD card state, etc.) */
    dvr_api_get_status();

    /* Pre-fetch version and TF capacity into cache */
    dvr_api_get_id();
    dvr_api_get_tf_capacity_query();

    dvr_set_sensor_switch_enable(0);

    printf("DVR: dvr_page_create done, preview on, window=(0,0,1024,500)\n");

    lv_screen_load(dvr_screen);
    return dvr_screen;
}

/*
 * dvr_page_destroy — Delete the DVR screen.
 */
void dvr_page_destroy(void)
{
    if (dvr_screen) {
        lv_obj_delete(dvr_screen);
        dvr_screen = NULL;
    }
}
