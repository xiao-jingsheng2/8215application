#include "ui_font_chinese_16.h"

#if (defined(__GNUC__) || defined(__clang__)) && !defined(_MSC_VER)
#define UI_FONT_WEAK_SUPPORTED 1
#else
#define UI_FONT_WEAK_SUPPORTED 0
#endif

#if UI_FONT_WEAK_SUPPORTED
extern const lv_font_t lv_font_source_han_sans_sc_16_full_cjk __attribute__((weak));
#endif

#if defined(LV_FONT_SOURCE_HAN_SANS_SC_16_CJK) && LV_FONT_SOURCE_HAN_SANS_SC_16_CJK
#if UI_FONT_WEAK_SUPPORTED
extern const lv_font_t lv_font_source_han_sans_sc_16_cjk __attribute__((weak));
#else
LV_FONT_DECLARE(lv_font_source_han_sans_sc_16_cjk);
#endif
#endif

#if defined(LV_FONT_SOURCE_HAN_SANS_SC_14_CJK) && LV_FONT_SOURCE_HAN_SANS_SC_14_CJK
#if UI_FONT_WEAK_SUPPORTED
extern const lv_font_t lv_font_source_han_sans_sc_14_cjk __attribute__((weak));
#else
LV_FONT_DECLARE(lv_font_source_han_sans_sc_14_cjk);
#endif
#endif

#if defined(LV_FONT_SIMSUN_16_CJK) && LV_FONT_SIMSUN_16_CJK
#if UI_FONT_WEAK_SUPPORTED
extern const lv_font_t lv_font_simsun_16_cjk __attribute__((weak));
#else
LV_FONT_DECLARE(lv_font_simsun_16_cjk);
#endif
#endif

#if defined(LV_FONT_SIMSUN_14_CJK) && LV_FONT_SIMSUN_14_CJK
#if UI_FONT_WEAK_SUPPORTED
extern const lv_font_t lv_font_simsun_14_cjk __attribute__((weak));
#else
LV_FONT_DECLARE(lv_font_simsun_14_cjk);
#endif
#endif

const lv_font_t * ui_font_chinese_16_get(void)
{
#if UI_FONT_WEAK_SUPPORTED
    if(&lv_font_source_han_sans_sc_16_full_cjk != NULL) {
        return &lv_font_source_han_sans_sc_16_full_cjk;
    }
#endif

#if defined(LV_FONT_SOURCE_HAN_SANS_SC_16_CJK) && LV_FONT_SOURCE_HAN_SANS_SC_16_CJK
#if UI_FONT_WEAK_SUPPORTED
    if(&lv_font_source_han_sans_sc_16_cjk != NULL) {
#endif
        return &lv_font_source_han_sans_sc_16_cjk;
#if UI_FONT_WEAK_SUPPORTED
    }
#endif
#endif

#if defined(LV_FONT_SOURCE_HAN_SANS_SC_14_CJK) && LV_FONT_SOURCE_HAN_SANS_SC_14_CJK
#if UI_FONT_WEAK_SUPPORTED
    if(&lv_font_source_han_sans_sc_14_cjk != NULL) {
#endif
        return &lv_font_source_han_sans_sc_14_cjk;
#if UI_FONT_WEAK_SUPPORTED
    }
#endif
#endif

#if defined(LV_FONT_SIMSUN_16_CJK) && LV_FONT_SIMSUN_16_CJK
#if UI_FONT_WEAK_SUPPORTED
    if(&lv_font_simsun_16_cjk != NULL) {
#endif
        return &lv_font_simsun_16_cjk;
#if UI_FONT_WEAK_SUPPORTED
    }
#endif
#endif

#if defined(LV_FONT_SIMSUN_14_CJK) && LV_FONT_SIMSUN_14_CJK
#if UI_FONT_WEAK_SUPPORTED
    if(&lv_font_simsun_14_cjk != NULL) {
#endif
        return &lv_font_simsun_14_cjk;
#if UI_FONT_WEAK_SUPPORTED
    }
#endif
#endif

    return LV_FONT_DEFAULT;
}
