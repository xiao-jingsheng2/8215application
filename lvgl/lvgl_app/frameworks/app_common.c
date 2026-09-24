/**
 * @file app_common.c
 * @brief Common APP utility function implementations
 */

#include "app_common.h"

const char *app_id_to_str(app_id_t id)
{
    switch (id) {
        case APP_HOME:                return "HOME";
        case APP_CARPLAY:             return "CARPLAY";
        case APP_CARPLAY_SETTINGS:    return "CARPLAY_SETTINGS";
        case APP_ANDROID_AUTO:        return "ANDROID_AUTO";
        case APP_ANDROID_AUTO_SETTINGS: return "ANDROID_AUTO_SETTINGS";
        case APP_VIDEO:               return "VIDEO";
        case APP_RVC:                 return "RVC";
        case APP_MUSIC:               return "MUSIC";
        case APP_SETTINGS:            return "SETTINGS";
        case APP_BLUETOOTH:           return "BLUETOOTH";
        default:                      return "UNKNOWN";
    }
}

const char *app_priority_to_str(app_priority_t priority)
{
    switch (priority) {
        case APP_PRIORITY_BACKGROUND: return "BACKGROUND";
        case APP_PRIORITY_LOW:        return "LOW";
        case APP_PRIORITY_MEDIUM:     return "MEDIUM";
        case APP_PRIORITY_HIGH:       return "HIGH";
        case APP_PRIORITY_SYSTEM:     return "SYSTEM";
        default:                      return "UNKNOWN";
    }
}
