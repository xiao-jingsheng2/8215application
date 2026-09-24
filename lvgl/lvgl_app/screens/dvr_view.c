/*
 * dvr_view.c — DVR UI state machine (LVGL 9.x port)
 *
 * Ported from AWTK dvr_view.c (dc002-0617 branch, AMT630HV100).
 * Layout replaces dvr_page.xml; state machine logic is 1:1 equivalent.
 *
 * Target: lvgl/lvgl_app/screens/dvr_view.c
 * Source: HCN_DC001/src/view/home_view/dvr_view.c (826 lines)
 *         HCN_DC001/design/default/ui/dvr_page.xml
 *
 * AWTK→LVGL mapping applied:
 *   widget_t*                    → lv_obj_t*
 *   widget_set_visible(w,T/F)   → lv_obj_remove/add_flag(w, LV_OBJ_FLAG_HIDDEN)
 *   widget_set_text_utf8(w,s)   → lv_label_set_text(w, s)
 *   widget_set_state(w,"sel")   → lv_obj_add_state(w, LV_STATE_CHECKED)
 *   widget_move(w,x,y)          → lv_obj_set_pos(w, x, y)
 *   progress_bar_set_value(w,v) → lv_bar_set_value(w, v, LV_ANIM_OFF)
 *   timer_add/remove             → lv_timer_create/delete
 *   widget_lookup(parent,"n",T) → direct pointer (created in build_layout)
 *   widget_set_style_color(...)  → lv_obj_set_style_text_color(...)
 *   widget_set_prop_str(..IMAGE) → lv_image_set_src(w, &img)
 */

#include "dvr_view.h"
#include "../images/dvr_images.h"

/* dvr_api.h is the same AWTK backend — include unchanged */
#include "dvr_api.h"

/* Forward declarations */
static void show_sub(dvr_sub_page_e sub);
static void del_poll_stop(void);
static void dvr_request_file_list(void);

/* ================================================================
 * Helper macros: LVGL visibility
 * ================================================================ */
#define W_SHOW(w)  do { if(w) lv_obj_remove_flag((w), LV_OBJ_FLAG_HIDDEN); } while(0)
#define W_HIDE(w)  do { if(w) lv_obj_add_flag((w), LV_OBJ_FLAG_HIDDEN); } while(0)
#define W_VISIBLE(w, vis)  do { if(vis) { W_SHOW(w); } else { W_HIDE(w); } } while(0)

/* ================================================================
 * Widgets (created in build_layout, parallel to AWTK widget_lookup)
 * ================================================================ */
static lv_obj_t *dvr_main_view    = NULL;
static lv_obj_t *dvr_idle_view    = NULL;
static lv_obj_t *dvr_list_view    = NULL;
static lv_obj_t *dvr_setting_view = NULL;
static lv_obj_t *dvr_popup_view   = NULL;
static lv_obj_t *dvr_dock_bar     = NULL;
static lv_obj_t *dvr_cam_dock     = NULL;
static lv_obj_t *dvr_cam_tab_sel  = NULL;
static lv_obj_t *dvr_main_bg      = NULL;

static lv_obj_t *dock_btn[DVR_DOCK_BTN_MAX] = {0};

static lv_obj_t *file_name_w[DVR_FILE_ITEM_MAX]  = {0};
static lv_obj_t *file_icon_w[DVR_FILE_ITEM_MAX]  = {0};
static lv_obj_t *file_view_w[DVR_FILE_ITEM_MAX]  = {0};
static lv_obj_t *file_del_w[DVR_FILE_ITEM_MAX]   = {0};

static lv_obj_t *list_tab_sel    = NULL;
static lv_obj_t *tab_label_left  = NULL;
static lv_obj_t *tab_label_right = NULL;
static lv_obj_t *set_fmt_sel     = NULL;
static lv_obj_t *set_ver_text    = NULL;
static lv_obj_t *set_loop_sel    = NULL;
static lv_obj_t *storage_bar     = NULL;
static lv_obj_t *storage_text    = NULL;
static lv_obj_t *popup_loading_w = NULL;
static lv_obj_t *list_no_sd_label= NULL;

/* Idle view widgets */
static lv_obj_t *idle_tab_sel    = NULL;
static lv_obj_t *idle_tab_left   = NULL;
static lv_obj_t *idle_tab_right  = NULL;

/* Popup sub-widgets */
static lv_obj_t *popup_title     = NULL;
static lv_obj_t *popup_confirm_bg= NULL;
static lv_obj_t *popup_cancel_bg = NULL;
static lv_obj_t *popup_confirm_lbl=NULL;
static lv_obj_t *popup_cancel_lbl =NULL;

/* Settings row highlight backgrounds */
static lv_obj_t *set_row_bg[DVR_SET_ROW_MAX] = {0};

/* ================================================================
 * State (identical to AWTK original)
 * ================================================================ */
static dvr_sub_page_e cur_sub = DVR_SUB_MAIN;
static int dock_focus   = DVR_DOCK_PREVIEW;
static int cam_focus    = DVR_CAM_FRONT;
static int list_focus   = 0;
static int list_tab     = DVR_TAB_FRONT;
static int list_mode    = 0;
static int set_focus    = DVR_SET_VERSION;
static int popup_focus  = 0;
static int set_loop_val  = 0;
static int set_edit_val  = 0;
static int set_fmt_focus = DVR_FMT_SD_FORMAT;
static int list_count    = 0;
static int list_offset   = 0;

typedef enum { POP_FILE_DEL=0, POP_FORMAT=1, POP_NO_SD=2, POP_FACTORY_RST=3 } pop_src_e;
static pop_src_e popup_src = POP_FILE_DEL;

static int pb_paused = 0, pb_idx = 0, pb_mode = 0;

#define LIST_ACT_PLAY    0
#define LIST_ACT_DELETE  1
static int list_act_focus = LIST_ACT_PLAY;

static int list_fetch_pending = 0;
static lv_timer_t *list_poll_timer = NULL;
#define LIST_POLL_INTERVAL_MS  100
#define LIST_POLL_MAX_RETRIES  30
static int list_poll_retries = 0;

static lv_timer_t *loading_timer = NULL;
#define LOADING_POLL_INTERVAL_MS  200
#define LOADING_POLL_MAX_RETRIES  30
static int loading_poll_retries = 0;
static int loading_anim_frame   = 0;

static lv_timer_t *del_poll_timer = NULL;
#define DEL_POLL_INTERVAL_MS  200
#define DEL_POLL_MAX_RETRIES  25
static int del_poll_retries = 0;

static dvr_sub_page_e no_sd_return_sub = DVR_SUB_MAIN;

static inline uint8_t list_api_mode(void) { return (uint8_t)(list_mode * 2 + list_tab); }

/* Loading spinner image table */
static const lv_image_dsc_t *loading_imgs[8] = {
    &ui_img_dvr_loading_0_png, &ui_img_dvr_loading_1_png,
    &ui_img_dvr_loading_2_png, &ui_img_dvr_loading_3_png,
    &ui_img_dvr_loading_4_png, &ui_img_dvr_loading_5_png,
    &ui_img_dvr_loading_6_png, &ui_img_dvr_loading_7_png,
};

/* Dock button icon tables (normal / selected) */
static const lv_image_dsc_t *dock_icon_n[DVR_DOCK_BTN_MAX] = {
    &ui_img_icon_camera_n_png, &ui_img_icon_playback_n_png,
    &ui_img_icon_photo_n_png,  &ui_img_icon_settings_n_png,
};
static const lv_image_dsc_t *dock_icon_p[DVR_DOCK_BTN_MAX] = {
    &ui_img_icon_camera_p_png, &ui_img_icon_playback_p_png,
    &ui_img_icon_photo_p_png,  &ui_img_icon_settings_p_png,
};

/* ================================================================
 * Visibility control — show_sub (direct port of AWTK show_sub)
 * ================================================================ */
static void show_sub(dvr_sub_page_e sub)
{
    dvr_sub_page_e prev = cur_sub;
    cur_sub = sub;

    /* Stop list/del timers when leaving list area */
    if ((prev == DVR_SUB_LIST || prev == DVR_SUB_LIST_ACT) &&
        sub != DVR_SUB_LIST && sub != DVR_SUB_LIST_ACT) {
        if (list_poll_timer) {
            lv_timer_delete(list_poll_timer);
            list_poll_timer = NULL;
            list_fetch_pending = 0;
        }
        del_poll_stop();
    }

    /* Preview enable/disable */
    int pv_prev = (prev == DVR_SUB_MAIN || prev == DVR_SUB_CAM_SW || prev == DVR_SUB_PLAYBACK);
    int pv_next = (sub  == DVR_SUB_MAIN || sub  == DVR_SUB_CAM_SW || sub  == DVR_SUB_PLAYBACK);
    if (pv_prev && !pv_next) {
        dvr_api_set_preview_enable(0);
        printf("DVR: preview off (sub=%d)\n", sub);
    }

    /* Visibility flags — exact replica of AWTK original */
    int mv = (sub == DVR_SUB_MAIN || sub == DVR_SUB_CAM_SW || sub == DVR_SUB_PLAYBACK);
    W_VISIBLE(dvr_main_view,    mv);
    W_VISIBLE(dvr_main_bg,      sub == DVR_SUB_MAIN);
    W_VISIBLE(dvr_idle_view,    sub == DVR_SUB_LIST_IDLE || sub == DVR_SUB_LIST_SEL);
    W_VISIBLE(dvr_list_view,    sub == DVR_SUB_LIST || sub == DVR_SUB_LIST_ACT);
    W_VISIBLE(dvr_setting_view, sub == DVR_SUB_SETTING || sub == DVR_SUB_SET_EDIT);
    W_VISIBLE(dvr_popup_view,   sub == DVR_SUB_POPUP || sub == DVR_SUB_LOADING);
    W_VISIBLE(dvr_dock_bar,     sub == DVR_SUB_MAIN);
    W_VISIBLE(dvr_cam_dock,     sub == DVR_SUB_CAM_SW);

    if (!pv_prev && pv_next) {
        dvr_api_set_preview_enable(1);
        printf("DVR: preview on (sub=%d)\n", sub);
    }
}

/* ================================================================
 * Highlight helpers — exact port of AWTK logic
 * ================================================================ */
static void hl_dock(int i)
{
    for (int n = 0; n < DVR_DOCK_BTN_MAX; n++) {
        if (dock_btn[n]) {
            lv_image_set_src(dock_btn[n], (n == i) ? dock_icon_p[n] : dock_icon_n[n]);
        }
    }
    dock_focus = i;
}

static void hl_cam(int i)
{
    cam_focus = i;
    if (dvr_cam_tab_sel) lv_obj_set_pos(dvr_cam_tab_sel, i * 341, 0);
}

static void hl_list(int i)
{
    for (int n = 0; n < DVR_FILE_ITEM_MAX; n++) {
        if (file_name_w[n]) {
            lv_obj_set_style_text_color(file_name_w[n],
                (n == i) ? lv_color_hex(0x00FF00) : lv_color_hex(0x083557),
                LV_PART_MAIN);
        }
        if (file_view_w[n]) lv_image_set_src(file_view_w[n], &ui_img_icon_view_n_png);
        if (file_del_w[n])  lv_image_set_src(file_del_w[n],  &ui_img_icon_delete_n_png);
    }
}

static void hl_list_act(int act)
{
    list_act_focus = act;
    int i = list_focus;
    if (i >= 0 && i < DVR_FILE_ITEM_MAX) {
        if (file_view_w[i])
            lv_image_set_src(file_view_w[i],
                (act == LIST_ACT_PLAY) ? &ui_img_icon_view_p_png : &ui_img_icon_view_n_png);
        if (file_del_w[i])
            lv_image_set_src(file_del_w[i],
                (act == LIST_ACT_DELETE) ? &ui_img_icon_delete_p_png : &ui_img_icon_delete_n_png);
    }
}

static void hl_tab(int t)
{
    list_tab = t;
    if (list_tab_sel) lv_obj_set_pos(list_tab_sel, t ? 512 : 0, 0);
    if (tab_label_left && tab_label_right) {
        if (list_mode == 0) {
            lv_label_set_text(tab_label_left,  "Front Video");
            lv_label_set_text(tab_label_right, "Rear Video");
        } else {
            lv_label_set_text(tab_label_left,  "Front Photo");
            lv_label_set_text(tab_label_right, "Rear Photo");
        }
    }
}

static void hl_idle_tab(int t)
{
    if (idle_tab_sel)   lv_obj_set_pos(idle_tab_sel, t ? 512 : 0, 0);
    if (idle_tab_left && idle_tab_right) {
        if (list_mode == 0) {
            lv_label_set_text(idle_tab_left,  "Front Video");
            lv_label_set_text(idle_tab_right, "Rear Video");
        } else {
            lv_label_set_text(idle_tab_left,  "Front Photo");
            lv_label_set_text(idle_tab_right, "Rear Photo");
        }
    }
}

static void hl_set(int r)
{
    if (!dvr_setting_view) return;
    for (int i = 0; i < DVR_SET_ROW_MAX; i++) {
        if (set_row_bg[i]) {
            lv_image_set_src(set_row_bg[i],
                (i == r) ? &ui_img_settings_btn1_p_png : &ui_img_settings_btn1_n_png);
        }
    }
}

static void hl_fmt(int f)
{
    if (set_fmt_sel) lv_obj_set_pos(set_fmt_sel, (f == 0) ? 285 : 615, 28);
    set_fmt_focus = f;
}

static void hl_loop(int v)
{
    if (set_loop_sel) lv_obj_set_pos(set_loop_sel, 278 + v * 220, 28);
}

static void hl_popup(int f)
{
    if (!dvr_popup_view) return;
    W_VISIBLE(popup_loading_w, 0);

    if (popup_src == POP_NO_SD) {
        W_SHOW(popup_confirm_bg);
        lv_image_set_src(popup_confirm_bg, &ui_img_pop_up_btn_p_png);
        W_SHOW(popup_confirm_lbl);
        lv_label_set_text(popup_confirm_lbl, "OK");
        W_HIDE(popup_cancel_bg);
        W_HIDE(popup_cancel_lbl);
    } else {
        W_SHOW(popup_confirm_bg);
        lv_image_set_src(popup_confirm_bg,
            (f == 0) ? &ui_img_pop_up_btn_p_png : &ui_img_pop_up_btn_n_png);
        W_SHOW(popup_cancel_bg);
        lv_image_set_src(popup_cancel_bg,
            (f == 1) ? &ui_img_pop_up_btn_p_png : &ui_img_pop_up_btn_n_png);
        W_SHOW(popup_confirm_lbl);
        lv_label_set_text(popup_confirm_lbl, "OK");
        W_SHOW(popup_cancel_lbl);
        lv_label_set_text(popup_cancel_lbl, "Cancel");
    }
}

/* ================================================================
 * Loading popup helpers
 * ================================================================ */
static void popup_show_loading(void)
{
    if (!dvr_popup_view) return;
    if (popup_title) lv_label_set_text(popup_title, "Loading...");
    W_HIDE(popup_confirm_bg);
    W_HIDE(popup_cancel_bg);
    W_HIDE(popup_confirm_lbl);
    W_HIDE(popup_cancel_lbl);
    if (popup_loading_w) {
        W_SHOW(popup_loading_w);
        lv_image_set_src(popup_loading_w, &ui_img_dvr_loading_0_png);
    }
}

static void show_no_sd_popup(dvr_sub_page_e return_to)
{
    if (popup_title) lv_label_set_text(popup_title, "No SD Card");
    popup_src = POP_NO_SD;
    no_sd_return_sub = return_to;
    popup_focus = 0;
    show_sub(DVR_SUB_POPUP);
    hl_popup(0);
    printf("DVR: no SD card alert\n");
}

/* ================================================================
 * Loading timer
 * ================================================================ */
static void loading_stop(void)
{
    if (loading_timer) { lv_timer_delete(loading_timer); loading_timer = NULL; }
}

static void loading_fill_settings(void)
{
    char vbuf[32];
    if (dvr_api_get_version(vbuf, sizeof(vbuf))) {
        if (set_ver_text) lv_label_set_text(set_ver_text, vbuf);
    } else {
        if (set_ver_text) lv_label_set_text(set_ver_text, "N/A");
    }
    uint32_t total_kib = 0, free_kib = 0;
    if (dvr_api_get_tf_capacity(&total_kib, &free_kib)) {
        uint32_t used_kib = (total_kib > free_kib) ? (total_kib - free_kib) : 0;
        dvr_setting_update_storage_kib(used_kib, total_kib);
    } else {
        if (storage_text) lv_label_set_text(storage_text, "N/A");
        if (storage_bar)  lv_bar_set_value(storage_bar, 0, LV_ANIM_OFF);
    }
}

static void on_loading_poll_timer(lv_timer_t *t)
{
    (void)t;
    loading_anim_frame = (loading_anim_frame + 1) & 7;
    if (popup_loading_w)
        lv_image_set_src(popup_loading_w, loading_imgs[loading_anim_frame]);

    int tf_ok = dvr_api_get_tf_capacity(NULL, NULL);
    if (tf_ok || ++loading_poll_retries >= LOADING_POLL_MAX_RETRIES) {
        loading_stop();
        loading_fill_settings();
        show_sub(DVR_SUB_SETTING);
        set_focus = DVR_SET_VERSION;
        hl_set(DVR_SET_VERSION);
        hl_loop(set_loop_val);
        hl_fmt(set_fmt_focus);
        printf("DVR: loading done (tf=%d retries=%d) -> settings\n", tf_ok, loading_poll_retries);
    }
}

static void enter_settings(void)
{
    int tf_ok = dvr_api_get_tf_capacity(NULL, NULL);
    if (tf_ok) {
        loading_fill_settings();
        show_sub(DVR_SUB_SETTING);
        set_focus = DVR_SET_VERSION;
        hl_set(DVR_SET_VERSION); hl_loop(set_loop_val); hl_fmt(set_fmt_focus);
        printf("DVR: enter settings (cache hit)\n");
        return;
    }
    loading_stop();
    loading_poll_retries = 0;
    loading_anim_frame = 0;
    dvr_api_rec_stop();
    dvr_api_get_id();
    dvr_api_get_tf_capacity_query();
    popup_show_loading();
    show_sub(DVR_SUB_LOADING);
    loading_timer = lv_timer_create(on_loading_poll_timer, LOADING_POLL_INTERVAL_MS, NULL);
    printf("DVR: enter settings (cache miss, loading started)\n");
}

/* ================================================================
 * Delete-then-refresh timer
 * ================================================================ */
static void del_poll_stop(void)
{
    if (del_poll_timer) { lv_timer_delete(del_poll_timer); del_poll_timer = NULL; }
}

static int del_pending_fi = -1;

static void del_optimistic_remove(int fi)
{
    if (fi < 0 || fi >= list_count) return;
    list_count--;
    for (int i = fi - list_offset; i < DVR_FILE_ITEM_MAX - 1; i++) {
        if (i >= 0 && i + 1 < DVR_FILE_ITEM_MAX && file_name_w[i] && file_name_w[i+1]) {
            const char *tmp = lv_label_get_text(file_name_w[i+1]);
            lv_label_set_text(file_name_w[i], tmp ? tmp : "");
        }
    }
    int last = DVR_FILE_ITEM_MAX - 1;
    if (last >= 0 && file_name_w[last]) lv_label_set_text(file_name_w[last], "");
    if (list_focus >= list_count && list_focus > 0) list_focus--;
    hl_list(list_focus);
    printf("DVR: UI optimistic remove fi=%d, visible count=%d\n", fi, list_count);
}

static void on_del_poll_timer(lv_timer_t *t)
{
    (void)t;
    if (cur_sub != DVR_SUB_LIST && cur_sub != DVR_SUB_LIST_ACT) {
        del_poll_stop(); del_pending_fi = -1;
        printf("DVR: del verify aborted (left list)\n");
        return;
    }
    del_poll_retries++;
    if (del_poll_retries <= 3) return;
    if (del_poll_retries == 4) {
        printf("DVR: del verify: requesting fresh list\n");
        dvr_file_list_clear();
        dvr_request_file_list();
        return;
    }
    del_poll_stop(); del_pending_fi = -1;
}

static void del_then_refresh(int fi)
{
    del_poll_stop();
    del_pending_fi = fi;
    del_poll_retries = 0;
    del_optimistic_remove(fi);
    del_poll_timer = lv_timer_create(on_del_poll_timer, DEL_POLL_INTERVAL_MS, NULL);
    printf("DVR: del fi=%d, UI updated, background verify started\n", fi);
}

/* ================================================================
 * File list data helpers (identical logic to AWTK original)
 * ================================================================ */
static void on_filelist_poll_timer(lv_timer_t *t)
{
    (void)t;
    if (dvr_api_is_filelist_ready()) {
        dvr_api_clear_filelist_ready();
        list_fetch_pending = 0;
        lv_timer_delete(list_poll_timer); list_poll_timer = NULL;
        dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: filelist ready, populated\n");
        return;
    }
    if (++list_poll_retries >= LIST_POLL_MAX_RETRIES) {
        list_fetch_pending = 0;
        lv_timer_delete(list_poll_timer); list_poll_timer = NULL;
        dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: filelist poll timeout (%d retries)\n", LIST_POLL_MAX_RETRIES);
    }
}

static void dvr_request_file_list(void)
{
    if (list_poll_timer) { lv_timer_delete(list_poll_timer); list_poll_timer = NULL; }
    dvr_api_clear_filelist_ready();
    list_fetch_pending = 1; list_poll_retries = 0;
    dvr_api_get_list(list_api_mode());
    list_poll_timer = lv_timer_create(on_filelist_poll_timer, LIST_POLL_INTERVAL_MS, NULL);
    printf("DVR: file list requested mode=%d, poll started\n", list_api_mode());
}

static uint16_t get_count(void)
{
    switch (list_api_mode()) {
    case 0: return dvr_api_get_video_list_f_count();
    case 1: return dvr_api_get_video_list_r_count();
    case 2: return dvr_api_get_photo_list_f_count();
    case 3: return dvr_api_get_photo_list_r_count();
    default:return 0;
    }
}

#define DVR_FETCH_MAX 128
static uint8_t get_fname(int idx, char *out, int sz)
{
    static char buf[DVR_FETCH_MAX * DVR_NAME_MAX];
    uint16_t c = 0;
    switch (list_api_mode()) {
    case 0: c = dvr_api_get_video_list_f(buf, DVR_FETCH_MAX); break;
    case 1: c = dvr_api_get_video_list_r(buf, DVR_FETCH_MAX); break;
    case 2: c = dvr_api_get_photo_list_f(buf, DVR_FETCH_MAX); break;
    case 3: c = dvr_api_get_photo_list_r(buf, DVR_FETCH_MAX); break;
    default: return 0;
    }
    if (c > DVR_FETCH_MAX) c = DVR_FETCH_MAX;
    if (idx < 0 || idx >= (int)c) return 0;
    const char *s = buf + (size_t)idx * DVR_NAME_MAX;
    int l = (int)strlen(s);
    if (l >= sz) l = sz - 1;
    memcpy(out, s, l); out[l] = '\0';
    if (list_tab == DVR_TAB_REAR) {
        char *dot = strrchr(out, '.');
        if (dot != NULL && (int)strlen(out) + 2 < sz) {
            memmove(dot + 2, dot, strlen(dot) + 1);
            dot[0] = '_'; dot[1] = 'R';
        }
    }
    return 1;
}

/* ================================================================
 * Layout builder — programmatic equivalent of dvr_page.xml
 * Screen 1024×600, all panels stacked, visibility-controlled.
 * ================================================================ */

/* Helper: create a positioned image widget inside parent */
static lv_obj_t *mk_img(lv_obj_t *par, const lv_image_dsc_t *src,
                         int x, int y, int w, int h)
{
    lv_obj_t *img = lv_image_create(par);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, x, y);
    lv_obj_set_size(img, w, h);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return img;
}

/* Helper: create positioned label */
static lv_obj_t *mk_label(lv_obj_t *par, const char *txt,
                           int x, int y, int w, int h, int font_sz)
{
    lv_obj_t *lbl = lv_label_create(par);
    lv_label_set_text(lbl, txt);
    lv_obj_set_pos(lbl, x, y);
    lv_obj_set_size(lbl, w, h);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    (void)font_sz; /* font size applied via project-level style; kept for doc */
    return lbl;
}

/* Helper: create positioned container (replaces AWTK <view>) */
static lv_obj_t *mk_panel(lv_obj_t *par, int x, int y, int w, int h)
{
    lv_obj_t *p = lv_obj_create(par);
    lv_obj_remove_style_all(p);
    lv_obj_set_pos(p, x, y);
    lv_obj_set_size(p, w, h);
    lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

static void build_layout(lv_obj_t *parent)
{
    /* --- dvr_main_view (preview + docks) --- */
    dvr_main_view = mk_panel(parent, 0, 0, 1024, 600);

    dvr_main_bg = mk_img(dvr_main_view, &ui_img_dvr_bg_png, 0, 0, 1024, 500);

    /* Main dock bar */
    dvr_dock_bar = mk_panel(dvr_main_view, 0, 500, 1024, 100);
    mk_img(dvr_dock_bar, &ui_img_dock_bg_png, 0, 0, 1024, 100);
    dock_btn[0] = mk_img(dvr_dock_bar, &ui_img_icon_camera_n_png,   48,  0, 160, 100);
    mk_img(dvr_dock_bar, &ui_img_line_png, 256, 0, 1, 100);
    dock_btn[1] = mk_img(dvr_dock_bar, &ui_img_icon_playback_n_png, 304, 0, 160, 100);
    mk_img(dvr_dock_bar, &ui_img_line_png, 512, 0, 1, 100);
    dock_btn[2] = mk_img(dvr_dock_bar, &ui_img_icon_photo_n_png,    560, 0, 160, 100);
    mk_img(dvr_dock_bar, &ui_img_line_png, 768, 0, 1, 100);
    dock_btn[3] = mk_img(dvr_dock_bar, &ui_img_icon_settings_n_png, 816, 0, 160, 100);

    /* Camera sub-dock */
    dvr_cam_dock = mk_panel(dvr_main_view, 0, 500, 1024, 100);
    mk_img(dvr_cam_dock, &ui_img_dock_bg_png, 0, 0, 1024, 100);
    dvr_cam_tab_sel = mk_img(dvr_cam_dock, &ui_img_dock_selected_png, 0, 0, 341, 100);
    mk_img(dvr_cam_dock, &ui_img_line_png, 341, 0, 1, 100);
    mk_img(dvr_cam_dock, &ui_img_line_png, 682, 0, 1, 100);
    mk_label(dvr_cam_dock, "Front",  86, 19, 168, 61, 30);
    mk_label(dvr_cam_dock, "Rear",  427, 19, 168, 61, 30);
    mk_img(dvr_cam_dock, &ui_img_icon_photo_n_png, 812, 10, 80, 80);
    W_HIDE(dvr_cam_dock);

    /* --- dvr_idle_view (after playback back) --- */
    dvr_idle_view = mk_panel(parent, 0, 0, 1024, 600);
    mk_img(dvr_idle_view, &ui_img_dvr_bg_png, 0, 0, 1024, 500);
    lv_obj_t *idle_dock = mk_panel(dvr_idle_view, 0, 500, 1024, 100);
    mk_img(idle_dock, &ui_img_dock_bg_png, 0, 0, 1024, 100);
    idle_tab_sel = mk_img(idle_dock, &ui_img_dock_selected_png, 0, 0, 512, 100);
    lv_obj_set_size(idle_tab_sel, 512, 100); /* stretch to half */
    mk_img(idle_dock, &ui_img_line_png, 512, 0, 1, 100);
    idle_tab_left  = mk_label(idle_dock, "Front Video", 156, 19, 200, 61, 30);
    idle_tab_right = mk_label(idle_dock, "Rear Video",  668, 19, 200, 61, 30);
    W_HIDE(dvr_idle_view);

    /* --- dvr_list_view (file list) --- */
    dvr_list_view = mk_panel(parent, 0, 0, 1024, 600);
    mk_img(dvr_list_view, &ui_img_dvr_list_bg_png, 0, 0, 1024, 600);

    lv_obj_t *scroll = mk_panel(dvr_list_view, 44, 10, 936, 480);
    lv_obj_add_flag(scroll, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        int iy = i * 70;
        lv_obj_t *row = mk_panel(scroll, 0, iy, 600, 60);
        mk_img(row, &ui_img_list_bg_n_png, 0, 0, 600, 60);
        file_icon_w[i] = mk_img(row, &ui_img_list_icon_video_n_png, 10, 0, 60, 60);
        file_name_w[i] = mk_label(row, "", 80, 12, 320, 35, 28);
        lv_obj_set_style_text_color(file_name_w[i], lv_color_hex(0x083557), LV_PART_MAIN);
        file_view_w[i] = mk_img(row, &ui_img_icon_view_n_png,   390, 0, 100, 60);
        file_del_w[i]  = mk_img(row, &ui_img_icon_delete_n_png, 500, 0, 100, 60);
    }

    list_no_sd_label = mk_label(dvr_list_view, "No SD Card", 362, 200, 300, 60, 32);
    W_HIDE(list_no_sd_label);

    /* File list bottom tab bar */
    lv_obj_t *list_dock = mk_panel(dvr_list_view, 0, 500, 1024, 100);
    mk_img(list_dock, &ui_img_dock_bg_png, 0, 0, 1024, 100);
    list_tab_sel = mk_img(list_dock, &ui_img_dock_selected_png, 0, 0, 512, 100);
    lv_obj_set_size(list_tab_sel, 512, 100);
    mk_img(list_dock, &ui_img_line_png, 512, 0, 1, 100);
    tab_label_left  = mk_label(list_dock, "Front Video", 156, 19, 200, 61, 30);
    tab_label_right = mk_label(list_dock, "Rear Video",  668, 19, 200, 61, 30);
    W_HIDE(dvr_list_view);

    /* --- dvr_setting_view --- */
    dvr_setting_view = mk_panel(parent, 0, 0, 1024, 600);
    mk_img(dvr_setting_view, &ui_img_dvr_list_bg_png, 0, 0, 1024, 600);

    /* Row 0: Version */
    lv_obj_t *r0 = mk_panel(dvr_setting_view, 57, 110, 910, 80);
    set_row_bg[0] = mk_img(r0, &ui_img_settings_btn1_n_png, 0, 0, 250, 80);
    mk_img(r0, &ui_img_settings_btn3_n_png, 270, 0, 640, 80);
    mk_label(r0, "Version", 65, 18, 120, 43, 30);
    set_ver_text = mk_label(r0, "--", 310, 18, 560, 43, 28);

    /* Row 1: Loop Time */
    lv_obj_t *r1 = mk_panel(dvr_setting_view, 57, 210, 910, 80);
    set_row_bg[1] = mk_img(r1, &ui_img_settings_btn1_n_png, 0, 0, 250, 80);
    mk_img(r1, &ui_img_settings_btn2_n_png, 270, 0, 200, 80);
    mk_img(r1, &ui_img_settings_btn2_n_png, 490, 0, 200, 80);
    mk_img(r1, &ui_img_settings_btn2_n_png, 710, 0, 200, 80);
    mk_label(r1, "Loop", 65, 18, 120, 43, 30);
    mk_label(r1, "1 min", 303, 18, 80, 43, 30);
    set_loop_sel = mk_img(r1, &ui_img_settings_s_png, 278, 28, 24, 24);
    mk_label(r1, "2 min", 523, 18, 80, 43, 30);
    mk_label(r1, "3 min", 743, 18, 80, 43, 30);

    /* Row 2: Format (SD Format / Factory Reset) */
    lv_obj_t *r2 = mk_panel(dvr_setting_view, 57, 310, 910, 80);
    set_row_bg[2] = mk_img(r2, &ui_img_settings_btn1_n_png, 0, 0, 250, 80);
    mk_img(r2, &ui_img_settings_btn2w_n_png, 270, 0, 310, 80);
    mk_img(r2, &ui_img_settings_btn2w_n_png, 600, 0, 310, 80);
    mk_label(r2, "Format", 80, 18, 90, 43, 30);
    mk_label(r2, "Quick Format", 340, 18, 170, 43, 28);
    set_fmt_sel = mk_img(r2, &ui_img_settings_s_png, 285, 28, 24, 24);
    mk_label(r2, "Factory Reset", 670, 18, 170, 43, 28);

    /* Row 3: SD Card capacity */
    lv_obj_t *r3 = mk_panel(dvr_setting_view, 57, 410, 910, 80);
    set_row_bg[3] = mk_img(r3, &ui_img_settings_btn1_n_png, 0, 0, 250, 80);
    mk_img(r3, &ui_img_settings_btn3_n_png, 270, 0, 640, 80);
    mk_label(r3, "SD Card", 55, 18, 140, 43, 30);

    /* Storage progress bar */
    storage_bar = lv_bar_create(r3);
    lv_obj_set_pos(storage_bar, 300, 30);
    lv_obj_set_size(storage_bar, 360, 20);
    lv_bar_set_value(storage_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(storage_bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(storage_bar, lv_color_hex(0x00AA00), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(storage_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(storage_bar, LV_OPA_COVER, LV_PART_INDICATOR);

    storage_text = mk_label(r3, "--/--", 683, 19, 200, 43, 28);

    W_HIDE(dvr_setting_view);

    /* --- dvr_popup_view (centered) --- */
    dvr_popup_view = mk_panel(parent, 262, 165, 500, 270);
    mk_img(dvr_popup_view, &ui_img_pop_up_bg_png, 0, 0, 500, 270);
    popup_title = mk_label(dvr_popup_view, "Confirm?", 170, 60, 160, 46, 33);
    popup_loading_w = mk_img(dvr_popup_view, &ui_img_dvr_loading_0_png, 210, 90, 80, 80);
    W_HIDE(popup_loading_w);
    popup_confirm_bg  = mk_img(dvr_popup_view, &ui_img_pop_up_btn_n_png, 50,  170, 180, 60);
    popup_confirm_lbl = mk_label(dvr_popup_view, "OK",     112, 179, 56, 41, 30);
    popup_cancel_bg   = mk_img(dvr_popup_view, &ui_img_pop_up_btn_n_png, 270, 170, 180, 60);
    popup_cancel_lbl  = mk_label(dvr_popup_view, "Cancel", 332, 179, 56, 41, 30);
    W_HIDE(dvr_popup_view);
}

/* ================================================================
 * Public: Init / Destroy
 * ================================================================ */
int dvr_view_init(lv_obj_t *parent)
{
    if (!parent) return -1;

    build_layout(parent);

    /* Reset state — exact copy of AWTK init */
    dock_focus = DVR_DOCK_PREVIEW;
    cam_focus  = DVR_CAM_FRONT;
    list_focus = 0; list_tab = 0; list_mode = 0; list_count = 0; list_offset = 0;
    set_focus  = DVR_SET_VERSION; popup_focus = 0;
    set_loop_val = 0; set_edit_val = 0; set_fmt_focus = DVR_FMT_SD_FORMAT;
    popup_src = POP_FILE_DEL; pb_paused = 0; pb_idx = 0; pb_mode = 0;
    list_act_focus = LIST_ACT_PLAY;

    show_sub(DVR_SUB_MAIN);
    hl_dock(DVR_DOCK_PREVIEW);
    return 0;
}

void dvr_view_destroy(void)
{
    loading_stop();
    del_poll_stop();
    if (list_poll_timer) { lv_timer_delete(list_poll_timer); list_poll_timer = NULL; }
    /* Widget tree is deleted when parent screen is deleted by LVGL */
}

/* ================================================================
 * Key handlers — exact 1:1 port of AWTK state machine
 * ================================================================ */

/* ==== SET ==== */
void dvr_page_deal_key_set(void)
{
    switch (cur_sub) {
    case DVR_SUB_MAIN:
        switch (dock_focus) {
        case DVR_DOCK_PREVIEW:
            cam_focus = (dvr_get_view_mode() == 1) ? DVR_CAM_REAR : DVR_CAM_FRONT;
            show_sub(DVR_SUB_CAM_SW); hl_cam(cam_focus);
            printf("DVR: enter cam dock\n"); break;
        case DVR_DOCK_VIDEO_PB:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_MAIN); break; }
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode = 0; list_tab = DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter video cam select\n"); break;
        case DVR_DOCK_PHOTO_PB:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_MAIN); break; }
            dvr_api_rec_stop(); printf("DVR: rec stopped (enter file area)\n");
            list_mode = 1; list_tab = DVR_TAB_FRONT;
            show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
            printf("DVR: enter photo cam select\n"); break;
        case DVR_DOCK_SETTINGS:
            enter_settings(); break;
        default: break;
        } break;

    case DVR_SUB_CAM_SW:
        switch (cam_focus) {
        case DVR_CAM_FRONT:    dvr_api_view_switch(0); printf("DVR: cam->front\n"); break;
        case DVR_CAM_REAR:     dvr_api_view_switch(1); printf("DVR: cam->rear\n");  break;
        case DVR_CAM_SNAPSHOT:
            if (!dvr_get_sd_status()) { show_no_sd_popup(DVR_SUB_CAM_SW); }
            else { dvr_api_snap(); printf("DVR: snap!\n"); }
            break;
        default: break;
        } break;

    case DVR_SUB_LIST: {
        if (list_fetch_pending) { printf("DVR: list loading, ignoring SET\n"); break; }
        int fi = list_offset + list_focus;
        if (fi < list_count) {
            list_act_focus = LIST_ACT_PLAY;
            cur_sub = DVR_SUB_LIST_ACT;
            hl_list_act(LIST_ACT_PLAY);
            printf("DVR: list -> action select (row=%d)\n", fi);
        } else {
            printf("DVR: no file row=%d cnt=%d\n", fi, list_count);
        }
    } break;

    case DVR_SUB_LIST_ACT: {
        int fi = list_offset + list_focus;
        if (list_act_focus == LIST_ACT_PLAY) {
            pb_mode = list_api_mode(); pb_idx = fi; pb_paused = 0;
            uint16_t pb_file_no = (uint16_t)fi;
            { char fn[DVR_NAME_MAX];
              if (get_fname(fi, fn, sizeof(fn)) && strlen(fn) >= 8)
                  pb_file_no = (uint16_t)((fn[4]-'0')*1000 + (fn[5]-'0')*100
                                          + (fn[6]-'0')*10 + (fn[7]-'0')); }
            dvr_api_pb_start((uint8_t)pb_mode, pb_file_no);
            show_sub(DVR_SUB_PLAYBACK);
            printf("DVR: pb start mode=%d pos=%d file_no=%d\n", pb_mode, fi, pb_file_no);
        } else {
            popup_src = POP_FILE_DEL; popup_focus = 1;
            show_sub(DVR_SUB_POPUP); hl_popup(1);
            printf("DVR: action delete -> popup (row=%d)\n", fi);
        }
    } break;

    case DVR_SUB_SETTING:
        switch (set_focus) {
        case DVR_SET_VERSION: break; /* display-only */
        case DVR_SET_LOOP:   set_edit_val = set_loop_val; hl_loop(set_edit_val); show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_FORMAT: set_fmt_focus = DVR_FMT_SD_FORMAT; hl_fmt(set_fmt_focus); show_sub(DVR_SUB_SET_EDIT); break;
        case DVR_SET_SD_CARD: break; /* display-only */
        default: break;
        } break;

    case DVR_SUB_SET_EDIT:
        if (set_focus == DVR_SET_LOOP) {
            set_loop_val = set_edit_val;
            dvr_api_set_loop_time(set_loop_val + 1);
            hl_loop(set_loop_val);
        } else if (set_focus == DVR_SET_FORMAT) {
            if (set_fmt_focus == DVR_FMT_SD_FORMAT) { popup_src = POP_FORMAT; }
            else                                     { popup_src = POP_FACTORY_RST; }
            if (popup_title) lv_label_set_text(popup_title,
                (popup_src == POP_FORMAT) ? "Format SD?" : "Factory Reset?");
            popup_focus = 1; show_sub(DVR_SUB_POPUP); hl_popup(1);
            break;
        }
        show_sub(DVR_SUB_SETTING); hl_set(set_focus); break;

    case DVR_SUB_POPUP:
        if (popup_src == POP_NO_SD) {
            if (popup_title) lv_label_set_text(popup_title, "Confirm?");
            show_sub(no_sd_return_sub);
            if (no_sd_return_sub == DVR_SUB_MAIN) hl_dock(dock_focus);
            else if (no_sd_return_sub == DVR_SUB_CAM_SW) hl_cam(cam_focus);
            break;
        }
        if (popup_focus == 0) {
            if (popup_src == POP_FILE_DEL) {
                int fi = list_offset + list_focus;
                uint16_t par = (uint16_t)fi;
                { char fn[DVR_NAME_MAX];
                  if (get_fname(fi, fn, sizeof(fn)) && strlen(fn) >= 8)
                      par = (uint16_t)((fn[4]-'0')*1000 + (fn[5]-'0')*100
                                       + (fn[6]-'0')*10 + (fn[7]-'0')); }
                if (list_mode == 1) par |= 0x8000;
                if (list_tab == 1)  par |= 0x4000;
                dvr_api_clear_del_ack();
                dvr_send_normal_cmd(BD_CTRL_DEL_FILE, par);
                printf("DVR: del file_no=%d (fi=%d)\n", par & 0x3FFF, fi);
            } else if (popup_src == POP_FORMAT) {
                if (dvr_get_rec_status()) { dvr_api_rec_stop(); printf("DVR: rec stopped for format\n"); }
                dvr_api_format();
            } else if (popup_src == POP_FACTORY_RST) {
                if (dvr_get_rec_status()) { dvr_api_rec_stop(); printf("DVR: rec stopped for factory reset\n"); }
                dvr_api_restore_default();
                printf("DVR: factory reset executed\n");
            }
        }
        if (popup_src == POP_FORMAT || popup_src == POP_FACTORY_RST) {
            show_sub(DVR_SUB_SETTING); hl_set(set_focus);
        } else {
            int del_fi = list_offset + list_focus;
            show_sub(DVR_SUB_LIST);
            del_then_refresh(del_fi);
        }
        break;

    case DVR_SUB_PLAYBACK:
        pb_paused = pb_paused ? 0 : 1; dvr_api_pb_pause();
        printf("DVR: pb %s\n", pb_paused ? "paused" : "resumed"); break;

    case DVR_SUB_LIST_IDLE: break;

    case DVR_SUB_LIST_SEL:
        list_offset = 0; list_focus = 0;
        show_sub(DVR_SUB_LIST); dvr_file_list_clear();
        hl_list(0); hl_tab(list_tab); dvr_request_file_list();
        printf("DVR: cam select -> list (mode=%d tab=%d)\n", list_mode, list_tab);
        break;

    case DVR_SUB_LOADING: break;  /* ignore keys during loading */
    default: break;
    }
}

/* ==== BACK ==== */
void dvr_page_deal_key_back(void)
{
    switch (cur_sub) {
    case DVR_SUB_MAIN:
        W_HIDE(dvr_main_view);
        dvr_stop_preview();
        /* navigator_back() equivalent: caller handles screen transition */
        break;
    case DVR_SUB_CAM_SW:
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus); break;
    case DVR_SUB_LIST:
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab); break;
    case DVR_SUB_LIST_ACT:
        cur_sub = DVR_SUB_LIST;
        hl_list(list_focus);
        printf("DVR: action select -> list\n"); break;
    case DVR_SUB_SETTING:
        loading_stop();
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: rec restarted on exit settings -> main\n"); break;
    case DVR_SUB_SET_EDIT:
        if (set_focus == DVR_SET_LOOP) hl_loop(set_loop_val);
        else if (set_focus == DVR_SET_FORMAT) hl_fmt(set_fmt_focus);
        show_sub(DVR_SUB_SETTING); hl_set(set_focus); break;
    case DVR_SUB_POPUP:
        if (popup_src == POP_NO_SD) {
            if (popup_title) lv_label_set_text(popup_title, "Confirm?");
            show_sub(no_sd_return_sub);
            if (no_sd_return_sub == DVR_SUB_MAIN) hl_dock(dock_focus);
            else if (no_sd_return_sub == DVR_SUB_CAM_SW) hl_cam(cam_focus);
            break;
        }
        if (popup_src == POP_FORMAT || popup_src == POP_FACTORY_RST) {
            show_sub(DVR_SUB_SETTING); hl_set(set_focus);
        } else {
            show_sub(DVR_SUB_LIST); hl_list(list_focus);
        } break;
    case DVR_SUB_PLAYBACK:
        dvr_api_pb_stop(); pb_paused = 0;
        show_sub(DVR_SUB_LIST); dvr_file_list_populate(); hl_list(list_focus);
        printf("DVR: pb stopped -> list\n"); break;
    case DVR_SUB_LIST_IDLE:
        show_sub(DVR_SUB_LIST_SEL); hl_idle_tab(list_tab);
        printf("DVR: idle -> cam select\n"); break;
    case DVR_SUB_LIST_SEL:
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: rec restarted on exit -> main\n"); break;
    case DVR_SUB_LOADING:
        loading_stop();
        show_sub(DVR_SUB_MAIN); hl_dock(dock_focus);
        dvr_api_rec_start(); printf("DVR: loading cancelled, rec restarted -> main\n"); break;
    default: break;
    }
}

/* ==== UP ==== */
void dvr_page_deal_key_up(void)
{
    switch (cur_sub) {
    case DVR_SUB_MAIN:     if (dock_focus > 0) hl_dock(dock_focus - 1); break;
    case DVR_SUB_CAM_SW:   if (cam_focus > 0) hl_cam(cam_focus - 1); break;
    case DVR_SUB_LIST:
        if (list_focus > 0) { list_focus--; hl_list(list_focus); }
        else if (list_offset > 0) { list_offset--; dvr_file_list_populate(); hl_list(0); }
        else if (list_tab > 0) {
            list_tab--; list_offset = 0;
            dvr_file_list_clear(); dvr_request_file_list();
            list_focus = 0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if (set_focus > 0) { set_focus--; hl_set(set_focus); } break;
    case DVR_SUB_SET_EDIT:
        if (set_focus == DVR_SET_LOOP) { set_edit_val = (set_edit_val > 0) ? set_edit_val - 1 : 2; hl_loop(set_edit_val); }
        else if (set_focus == DVR_SET_FORMAT) { set_fmt_focus = set_fmt_focus ? 0 : 1; hl_fmt(set_fmt_focus); }
        break;
    case DVR_SUB_POPUP:    popup_focus = 0; hl_popup(0); break;
    case DVR_SUB_LIST_ACT:
        if (list_act_focus != LIST_ACT_PLAY) { list_act_focus = LIST_ACT_PLAY; hl_list_act(LIST_ACT_PLAY); }
        break;
    case DVR_SUB_LIST_SEL:
        if (list_tab > 0) { list_tab--; hl_idle_tab(list_tab); } break;
    case DVR_SUB_LOADING: break;
    default: break;
    }
}

/* ==== DOWN ==== */
void dvr_page_deal_key_down(void)
{
    switch (cur_sub) {
    case DVR_SUB_MAIN:     if (dock_focus < DVR_DOCK_BTN_MAX - 1) hl_dock(dock_focus + 1); break;
    case DVR_SUB_CAM_SW:   if (cam_focus < DVR_CAM_ITEM_MAX - 1) hl_cam(cam_focus + 1); break;
    case DVR_SUB_LIST:
        if (list_focus < DVR_FILE_ITEM_MAX - 1 && (list_offset + list_focus + 1) < list_count) {
            list_focus++; hl_list(list_focus);
        } else if ((list_offset + DVR_FILE_ITEM_MAX) < list_count) {
            list_offset++; dvr_file_list_populate(); hl_list(DVR_FILE_ITEM_MAX - 1);
        } else if (list_tab < DVR_TAB_MAX - 1) {
            list_tab++; list_offset = 0;
            dvr_file_list_clear(); dvr_request_file_list();
            list_focus = 0; hl_list(0); hl_tab(list_tab);
        }
        break;
    case DVR_SUB_SETTING:  if (set_focus < DVR_SET_ROW_MAX - 1) { set_focus++; hl_set(set_focus); } break;
    case DVR_SUB_SET_EDIT:
        if (set_focus == DVR_SET_LOOP) { set_edit_val = (set_edit_val + 1) % 3; hl_loop(set_edit_val); }
        else if (set_focus == DVR_SET_FORMAT) { set_fmt_focus = set_fmt_focus ? 0 : 1; hl_fmt(set_fmt_focus); }
        break;
    case DVR_SUB_POPUP:    popup_focus = 1; hl_popup(1); break;
    case DVR_SUB_LIST_ACT:
        if (list_act_focus != LIST_ACT_DELETE) { list_act_focus = LIST_ACT_DELETE; hl_list_act(LIST_ACT_DELETE); }
        break;
    case DVR_SUB_LIST_SEL:
        if (list_tab < DVR_TAB_MAX - 1) { list_tab++; hl_idle_tab(list_tab); } break;
    case DVR_SUB_LOADING: break;
    default: break;
    }
}

/* ================================================================
 * Public accessors
 * ================================================================ */
dvr_sub_page_e dvr_get_current_sub(void) { return cur_sub; }
void dvr_set_current_sub(dvr_sub_page_e sub) { show_sub(sub); }

void dvr_file_list_set_name(int i, const char *n)
{
    if (i >= 0 && i < DVR_FILE_ITEM_MAX && file_name_w[i] && n)
        lv_label_set_text(file_name_w[i], n);
}

void dvr_file_list_clear(void)
{
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++)
        if (file_name_w[i]) lv_label_set_text(file_name_w[i], "");
}

void dvr_file_list_populate(void)
{
    char fn[DVR_NAME_MAX]; uint16_t tot = get_count();
    if (list_offset > (int)tot - DVR_FILE_ITEM_MAX) list_offset = (int)tot - DVR_FILE_ITEM_MAX;
    if (list_offset < 0) list_offset = 0;
    list_count = (int)tot;
    for (int i = 0; i < DVR_FILE_ITEM_MAX; i++) {
        int fi = list_offset + i;
        if (fi < (int)tot && get_fname(fi, fn, sizeof(fn))) dvr_file_list_set_name(i, fn);
        else dvr_file_list_set_name(i, "");
    }
    printf("DVR: populate mode=%d tab=%d cnt=%d off=%d\n", list_mode, list_tab, tot, list_offset);
}

void dvr_setting_update_storage(int used, int total)
{
    char buf[32];
    if (storage_bar && total > 0) lv_bar_set_value(storage_bar, (used * 100) / total, LV_ANIM_OFF);
    if (storage_text) { snprintf(buf, sizeof(buf), "%dG/%dG", used, total); lv_label_set_text(storage_text, buf); }
}

void dvr_setting_update_storage_kib(uint32_t used_kib, uint32_t total_kib)
{
    char buf[48];
    if (total_kib == 0) {
        if (storage_bar)  lv_bar_set_value(storage_bar, 0, LV_ANIM_OFF);
        if (storage_text) lv_label_set_text(storage_text, "No SD");
        return;
    }
    double used_gib  = (double)used_kib  / (1024.0 * 1024.0);
    double total_gib = (double)total_kib / (1024.0 * 1024.0);
    int pct = (int)((uint64_t)used_kib * 100 / total_kib);
    if (pct > 100) pct = 100;
    if (storage_bar) lv_bar_set_value(storage_bar, pct, LV_ANIM_OFF);
    snprintf(buf, sizeof(buf), "%.1fG/%.1fG", used_gib, total_gib);
    if (storage_text) lv_label_set_text(storage_text, buf);
}

void dvr_setting_update_version(const char *ver)
{
    if (set_ver_text && ver) lv_label_set_text(set_ver_text, ver);
}
