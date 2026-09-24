#ifndef DVR_PAGE_H
#define DVR_PAGE_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *dvr_page_create(void);

void dvr_page_destroy(void);

#ifdef __cplusplus
}
#endif

#endif
