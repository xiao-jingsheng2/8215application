/*
 * dvr_page.h — DVR page screen management (LVGL 9.x port)
 *
 * Target: lvgl/lvgl_app/screens/dvr_page.h
 * Source: HCN_DC001/src/pages/dvr_page.c (implicit interface)
 */

#ifndef DVR_PAGE_H
#define DVR_PAGE_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and show the DVR screen.
 *        Initializes DVR view, starts preview, queries DVR status.
 *        Idempotent: if already created, loads the existing screen.
 * @return The DVR screen object.
 */
lv_obj_t *dvr_page_create(void);

/**
 * @brief Delete the DVR screen and free all resources.
 */
void dvr_page_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* DVR_PAGE_H */
