#ifndef AUDIO_SETTINGS_H
#define AUDIO_SETTINGS_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_STREAM_MEDIA   1
#define AUDIO_STREAM_NAVI    2
#define AUDIO_STREAM_BT      3
#define AUDIO_STREAM_BTRING  4

#define AUDIO_VOLUME_MIN     0
#define AUDIO_VOLUME_MAX     40

#define AUDIO_BASS_MIN      -14
#define AUDIO_BASS_MAX       14
#define AUDIO_TREBLE_MIN    -14
#define AUDIO_TREBLE_MAX     14

#define AUDIO_BALANCE_MIN   -20
#define AUDIO_BALANCE_MAX    20

#define AUDIO_LOUDNESS_MIN   0
#define AUDIO_LOUDNESS_MAX   20

#define AUDIO_EQ_OFF        0
#define AUDIO_EQ_ROCK       1
#define AUDIO_EQ_POP        2
#define AUDIO_EQ_LIVING     3
#define AUDIO_EQ_DANCE      4
#define AUDIO_EQ_CLASSICAL  5
#define AUDIO_EQ_SOFT       6
#define AUDIO_EQ_USER       7
#define AUDIO_EQ_MAX        8

#define AUDIO_REVERB_OFF         0
#define AUDIO_REVERB_LIVINGROOM  1
#define AUDIO_REVERB_HALL        2
#define AUDIO_REVERB_CONCERT     3
#define AUDIO_REVERB_CAVE        4
#define AUDIO_REVERB_BATHROOM    5
#define AUDIO_REVERB_ARENA       6
#define AUDIO_REVERB_MAX         7

int audio_settings_init(void);
void audio_settings_deinit(void);

int audio_set_volume(int stream_type, int index);
int audio_get_volume(int stream_type);

int audio_set_mute(bool mute);
bool audio_get_mute(void);

int audio_set_mic_mute(bool mute);
bool audio_get_mic_mute(void);

int audio_set_bass(int value);
int audio_get_bass(void);

int audio_set_treble(int value);
int audio_get_treble(void);

int audio_set_balance(int value);
int audio_get_balance(void);

int audio_set_loudness(int value);
int audio_get_loudness(void);

int audio_set_eq_type(int type);
int audio_get_eq_type(void);

int audio_set_reverb_type(int type);
int audio_get_reverb_type(void);

const char *audio_eq_type_name(int type);
const char *audio_reverb_type_name(int type);

#ifdef __cplusplus
}
#endif

#endif
