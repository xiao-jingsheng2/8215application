#include "audio_settings.h"
#include "AtcAudioSettings.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/types.h>


#define AUDIO_CONFIG_PATH       "/data/misc/audio"

static int s_media_volume = 20;
static int s_navi_volume = 20;
static int s_bt_volume = 20;
static int s_btring_volume = 20;
static bool s_mute = false;
static bool s_mic_mute = false;
static int s_bass = 0;
static int s_treble = 0;
static int s_balance = 0;
static int s_loudness = 0;
static int s_eq_type = 0;
static int s_reverb_type = 0;

static const int g_volumeValue[41] = {
    0, 0x5be, 0x672, 0x73b, 0x81d, 0x91a, 0xa37, 0xb76, 0xcdc, 0xe6e,
    0x1030, 0x122a, 0x1462, 0x16de, 0x19a9, 0x1cca, 0x204e, 0x243f, 0x28ab, 0x2da1,
    0x3333, 0x3972, 0x4074, 0x4852, 0x5125, 0x5b0c, 0x6628, 0x729f, 0x809b, 0x904d,
    0xa1e8, 0xb5aa, 0xcbd4, 0xe4b3, 0x1009b, 0x11feb, 0x1430c, 0x16a77, 0x196b2, 0x1c852, 0x20000
};

static const int g_BanalceValue[41] = {
    0x00000000, 0x00000672, 0x0000081D, 0x00000A37, 0x00000CDC,
    0x00001030, 0x00001462, 0x000019A9, 0x0000204E, 0x000028AB,
    0x00003333, 0x00004074, 0x00005125, 0x00006628, 0x0000809B,
    0x0000A1E8, 0x0000CBD4, 0x0001009B, 0x0001430C, 0x000196B2,
    0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000,
    0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000,
    0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000,
    0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000,
    0x00020000
};

static const int g_EQDryGain[29] = {
    0x00003314,0x0000394F,0x0000404D,0x00004826,0x000050F4,0x00005AD5,0x000065EA,
    0x00007259,0x0000804D,0x00008FF5,0x0000A186,0x0000B53B,0x0000CB59,0x0000E429,
    0x00010000,0x00011F3C,0x00014248,0x0001699C,0x000195BB,0x0001C73D,0x0001FEC9,
    0x00023D1C,0x0002830A,0x0002D181,0x0003298B,0x00038C52,0x0003FB27,0x00047782,
    0x0005030A
};

static const unsigned int g_EQBandGain[29] = {
    0xFFFF3315,0xFFFF3950,0xFFFF404E,0xFFFF4827,0xFFFF50F5,0xFFFF5AD6,0xFFFF65EB,
    0xFFFF725A,0xFFFF804E,0xFFFF8FF6,0xFFFFA187,0xFFFFB53C,0xFFFFCB5A,0xFFFFE42A,
    0x00000000,0x00001F3C,0x00004248,0x0000699C,0x000095BB,0x0000C73D,0x0000FEC9,
    0x00013D1C,0x0001830A,0x0001D181,0x0002298B,0x00028C52,0x0002FB27,0x00037782,
    0x0004030A
};

static const int gEQTypePos[8][11] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0,10, 8, 5, 0,-2, 0, 2, 4, 8,10},
    { 0,-2, 0, 4, 8,10, 8, 5, 1,-2,-4},
    { 0,14,12, 8, 2,-2, 0, 6,10, 8, 4},
    { 0,10, 9, 7, 4, 0,-3,-5,-3,-1, 0},
    { 0, 0, 0, 0, 0, 0, 0,-1,-2,-4,-5},
    { 0, 5, 4, 3, 2, 1,-1,-3,-5,-7,-8},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

static const MISC_EQ_GAIN_T gEQTypeGain[8] = {
    {0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
     0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
    {0x00010000,0x0002298B,0x0001830A,0x0000C73D,0x00000000,0xFFFFCB5A,
     0x00000000,0x00004248,0x000095BB,0x0001830A,0x0002298B},
    {0x00010000,0xFFFFCB5A,0x00000000,0x000095BB,0x0001830A,0x0002298B,
     0x0001830A,0x0000C73D,0x00001F3C,0xFFFFCB5A,0xFFFFA187},
    {0x00010000,0x0004030A,0x0002FB27,0x0001830A,0x00004248,0xFFFFCB5A,
     0x00000000,0x0000FEC9,0x0002298B,0x0001830A,0x000095BB},
    {0x00010000,0x0002298B,0x0001D181,0x00013D1C,0x000095BB,0x00000000,
     0xFFFFB53C,0xFFFF8FF6,0xFFFFB53C,0xFFFFE42A,0x00000000},
    {0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
     0x00000000,0xFFFFE42A,0xFFFFCB5A,0xFFFFA187,0xFFFF8FF6},
    {0x00010000,0x0000C73D,0x000095BB,0x0000699C,0x00004248,0x00001F3C,
     0xFFFFE42A,0xFFFFB53C,0xFFFF8FF6,0xFFFF725A,0xFFFF65EB},
    {0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,
     0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
};

static const MISC_LOUDNESS_GAIN_T g_LoudNessGain[21] = {
    {0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
    {0x00100000,0x00e0b3d9,0x000f4e20,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0be81,0x000f43b6,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0c9cb,0x000f38b1,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0d5c0,0x000f2d0a,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0e26a,0x000f20b6,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0efd5,0x000f13ae,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e0fe0b,0x000f05e5,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e10d19,0x000ef753,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e11d0a,0x000ee7eb,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e12dee,0x000ed7a2,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e13fd2,0x000ec66c,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e152c6,0x000eb43b,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e166d8,0x000ea103,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e17c1c,0x000e8cb6,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e192a2,0x000e7743,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e1aa7d,0x000e609d,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e1c3c3,0x000e48b2,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e1de87,0x000e2f72,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e1fae2,0x000e14cc,0x001f5636,0x00f0a807,0x007FFFFF},
    {0x00100000,0x00e218eb,0x000df8ad,0x001f5636,0x00f0a807,0x007FFFFF}
};

static const MISC_REVERB_COEF_T gAudSeReverbCoefTab[7] = {
    {
        0x180000, 0x7FFFFF,
        {{0x00000029, 0x0000001F, 0x00000017, 0x0000000D}, {0x00000029, 0x0000001F, 0x00000017, 0x0000000D}, {0x00000029, 0x0000001F, 0x00000017, 0x0000000D}}
    },
    {
        0x300000, 0x7FFFFF,
        {{0x0000004F, 0x0000003B, 0x00000025, 0x00000013}, {0x0000004F, 0x0000003B, 0x00000025, 0x00000013}, {0x0000004F, 0x0000003B, 0x00000025, 0x00000013}}
    },
    {
        0x200000, 0x7FFFFF,
        {{0x0000004F, 0x00000043, 0x0000003B, 0x0000002F}, {0x0000004F, 0x00000043, 0x0000003B, 0x0000002F}, {0x0000004F, 0x00000043, 0x0000003B, 0x0000002F}}
    },
    {
        0x600000, 0x7FFFFF,
        {{0x0000003B, 0x0000002F, 0x00000025, 0x00000017}, {0x0000003B, 0x0000002F, 0x00000025, 0x00000017}, {0x0000003B, 0x0000002F, 0x00000025, 0x00000017}}
    },
    {
        0x400000, 0x7FFFFF,
        {{0x00000013, 0x00000011, 0x0000000B, 0x00000007}, {0x00000013, 0x00000011, 0x0000000B, 0x00000007}, {0x00000013, 0x00000011, 0x0000000B, 0x00000007}}
    },
    {
        0x100000, 0x7FFFFF,
        {{0x0000004F, 0x00000049, 0x00000047, 0x00000043}, {0x0000004F, 0x00000049, 0x00000047, 0x00000043}, {0x0000004F, 0x00000049, 0x00000047, 0x00000043}}
    },
    {
        0x000000, 0x000000,
        {{0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0x00000000, 0x00000000, 0x00000000, 0x00000000}, {0x00000000, 0x00000000, 0x00000000, 0x00000000}}
    }
};

static const char *s_eq_names[8] = {
    "Off", "Rock", "Pop", "Living", "Dance", "Classical", "Soft", "User"
};

static const char *s_reverb_names[7] = {
    "Off", "Living Room", "Hall", "Concert", "Cave", "Bathroom", "Arena"
};

static void save_config_int(const char *key, int value)
{
    char path[128];
    char buf[16];
    snprintf(path, sizeof(path), "%s/%s", AUDIO_CONFIG_PATH, key);
    FILE *fp = fopen(path, "w");
    if (fp) {
        snprintf(buf, sizeof(buf), "%d", value);
        fputs(buf, fp);
        fclose(fp);
    }
}

static int load_config_int(const char *key, int default_val)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", AUDIO_CONFIG_PATH, key);
    FILE *fp = fopen(path, "r");
    if (fp) {
        char buf[16] = {0};
        fgets(buf, sizeof(buf), fp);
        fclose(fp);
        return atoi(buf);
    }
    return default_val;
}

static void ensure_config_dir(void)
{
    mkdir(AUDIO_CONFIG_PATH, 0755);
}

static int get_eq_gain_value(int eqPos, int bandIndex)
{
    int offset = 14 + eqPos;
    if (offset < 0) offset = 0;
    if (offset > 28) offset = 28;

    if (bandIndex == 0) {
        return g_EQDryGain[offset];
    }
    return (int)g_EQBandGain[offset];
}

static void apply_eq_with_bass_treble(int eqType, int bassVal, int trebleVal)
{
    int eqPos[11];
    int i;
    MISC_EQ_GAIN_T eqGain;

    if (eqType < 0 || eqType >= 8) eqType = 0;

    for (i = 0; i < 11; i++) {
        eqPos[i] = gEQTypePos[eqType][i];
    }

    for (i = 1; i < 4; i++) {
        eqPos[i] += bassVal;
    }
    for (i = 8; i < 11; i++) {
        eqPos[i] += trebleVal;
    }

    eqGain.u4Gain0 = get_eq_gain_value(eqPos[0], 0);
    eqGain.u4Gain1 = get_eq_gain_value(eqPos[1], 1);
    eqGain.u4Gain2 = get_eq_gain_value(eqPos[2], 2);
    eqGain.u4Gain3 = get_eq_gain_value(eqPos[3], 3);
    eqGain.u4Gain4 = get_eq_gain_value(eqPos[4], 4);
    eqGain.u4Gain5 = get_eq_gain_value(eqPos[5], 5);
    eqGain.u4Gain6 = get_eq_gain_value(eqPos[6], 6);
    eqGain.u4Gain7 = get_eq_gain_value(eqPos[7], 7);
    eqGain.u4Gain8 = get_eq_gain_value(eqPos[8], 8);
    eqGain.u4Gain9 = get_eq_gain_value(eqPos[9], 9);
    eqGain.u4Gain10 = get_eq_gain_value(eqPos[10], 10);

    SetEQType((EQTYPE_T)eqType, eqGain);
}

int audio_settings_init(void)
{
    int reverb_idx;

    ensure_config_dir();

    s_media_volume = load_config_int("media_volume", 20);
    s_navi_volume = load_config_int("navi_volume", 20);
    s_bt_volume = load_config_int("bt_volume", 20);
    s_btring_volume = load_config_int("btring_volume", 20);
    s_mute = load_config_int("mute", 0) ? true : false;
    s_mic_mute = load_config_int("mic_mute", 0) ? true : false;
    s_bass = load_config_int("bass", 0);
    s_treble = load_config_int("treble", 0);
    s_balance = load_config_int("balance", 0);
    s_loudness = load_config_int("loudness", 0);
    s_eq_type = load_config_int("eq_type", 0);
    s_reverb_type = load_config_int("reverb_type", 0);

    if (s_mute) {
        SetStreamTypeVolume(AUDIO_STREAM_MEDIA, 0);
    } else {
        SetStreamTypeVolume(AUDIO_STREAM_MEDIA, g_volumeValue[s_media_volume]);
    }
    SetStreamTypeVolume(AUDIO_STREAM_NAVI, g_volumeValue[s_navi_volume]);
    SetStreamTypeVolume(AUDIO_STREAM_BT, g_volumeValue[s_bt_volume]);
    SetStreamTypeVolume(AUDIO_STREAM_BTRING, g_volumeValue[s_btring_volume]);

    SetMicMute(s_mic_mute);

    apply_eq_with_bass_treble(s_eq_type, s_bass, s_treble);

    SetBalance(g_BanalceValue[s_balance + 20], BALANCE_FRONT_RIGHT);
    SetBalance(g_BanalceValue[20 - s_balance], BALANCE_FRONT_LEFT);
    SetBalance(g_BanalceValue[s_balance + 20], BALANCE_REAR_RIGHT);
    SetBalance(g_BanalceValue[20 - s_balance], BALANCE_REAR_LEFT);

    SetLoudNess(s_loudness, g_LoudNessGain[s_loudness]);

    switch (s_reverb_type) {
    case AUDIO_REVERB_OFF:         reverb_idx = 6; break;
    case AUDIO_REVERB_LIVINGROOM:  reverb_idx = 0; break;
    case AUDIO_REVERB_HALL:        reverb_idx = 1; break;
    case AUDIO_REVERB_CONCERT:     reverb_idx = 2; break;
    case AUDIO_REVERB_CAVE:        reverb_idx = 3; break;
    case AUDIO_REVERB_BATHROOM:    reverb_idx = 4; break;
    case AUDIO_REVERB_ARENA:       reverb_idx = 5; break;
    default:                       reverb_idx = 6; break;
    }
    SetReverbType((REVERBTYPE_T)s_reverb_type, gAudSeReverbCoefTab[reverb_idx]);

    return 0;
}

void audio_settings_deinit(void)
{
}

int audio_set_volume(int stream_type, int index)
{
    int *p_volume = NULL;
    const char *config_key = NULL;

    if (index < AUDIO_VOLUME_MIN) index = AUDIO_VOLUME_MIN;
    if (index > AUDIO_VOLUME_MAX) index = AUDIO_VOLUME_MAX;

    switch (stream_type) {
    case AUDIO_STREAM_MEDIA:
        p_volume = &s_media_volume;
        config_key = "media_volume";
        break;
    case AUDIO_STREAM_NAVI:
        p_volume = &s_navi_volume;
        config_key = "navi_volume";
        break;
    case AUDIO_STREAM_BT:
        p_volume = &s_bt_volume;
        config_key = "bt_volume";
        break;
    case AUDIO_STREAM_BTRING:
        p_volume = &s_btring_volume;
        config_key = "btring_volume";
        break;
    default:
        return -1;
    }

    *p_volume = index;
    save_config_int(config_key, index);

    if (s_mute) {
        return SetStreamTypeVolume(stream_type, 0);
    }
    return SetStreamTypeVolume(stream_type, g_volumeValue[index]);
}

int audio_get_volume(int stream_type)
{
    switch (stream_type) {
    case AUDIO_STREAM_MEDIA:   return s_media_volume;
    case AUDIO_STREAM_NAVI:   return s_navi_volume;
    case AUDIO_STREAM_BT:     return s_bt_volume;
    case AUDIO_STREAM_BTRING: return s_btring_volume;
    default: return 0;
    }
}

int audio_set_mute(bool mute)
{
    s_mute = mute;
    save_config_int("mute", mute ? 1 : 0);

    if (mute) {
        SetStreamTypeVolume(AUDIO_STREAM_MEDIA, 0);
    } else {
        SetStreamTypeVolume(AUDIO_STREAM_MEDIA, g_volumeValue[s_media_volume]);
    }
    return 0;
}

bool audio_get_mute(void)
{
    return s_mute;
}

int audio_set_mic_mute(bool mute)
{
    s_mic_mute = mute;
    save_config_int("mic_mute", mute ? 1 : 0);
    return SetMicMute(mute);
}

bool audio_get_mic_mute(void)
{
    return s_mic_mute;
}

int audio_set_bass(int value)
{
    if (value < AUDIO_BASS_MIN) value = AUDIO_BASS_MIN;
    if (value > AUDIO_BASS_MAX) value = AUDIO_BASS_MAX;

    s_bass = value;
    save_config_int("bass", value);
    apply_eq_with_bass_treble(s_eq_type, s_bass, s_treble);
    return 0;
}

int audio_get_bass(void)
{
    return s_bass;
}

int audio_set_treble(int value)
{
    if (value < AUDIO_TREBLE_MIN) value = AUDIO_TREBLE_MIN;
    if (value > AUDIO_TREBLE_MAX) value = AUDIO_TREBLE_MAX;

    s_treble = value;
    save_config_int("treble", value);
    apply_eq_with_bass_treble(s_eq_type, s_bass, s_treble);
    return 0;
}

int audio_get_treble(void)
{
    return s_treble;
}

int audio_set_balance(int value)
{
    if (value < AUDIO_BALANCE_MIN) value = AUDIO_BALANCE_MIN;
    if (value > AUDIO_BALANCE_MAX) value = AUDIO_BALANCE_MAX;

    s_balance = value;
    save_config_int("balance", value);

    SetBalance(g_BanalceValue[value + 20], BALANCE_FRONT_RIGHT);
    SetBalance(g_BanalceValue[20 - value], BALANCE_FRONT_LEFT);
    SetBalance(g_BanalceValue[value + 20], BALANCE_REAR_RIGHT);
    SetBalance(g_BanalceValue[20 - value], BALANCE_REAR_LEFT);
    return 0;
}

int audio_get_balance(void)
{
    return s_balance;
}

int audio_set_loudness(int value)
{
    if (value < AUDIO_LOUDNESS_MIN) value = AUDIO_LOUDNESS_MIN;
    if (value > AUDIO_LOUDNESS_MAX) value = AUDIO_LOUDNESS_MAX;

    s_loudness = value;
    save_config_int("loudness", value);
    return SetLoudNess(value, g_LoudNessGain[value]);
}

int audio_get_loudness(void)
{
    return s_loudness;
}

int audio_set_eq_type(int type)
{
    if (type < 0) type = 0;
    if (type >= AUDIO_EQ_MAX) type = 0;

    s_eq_type = type;
    save_config_int("eq_type", type);
    apply_eq_with_bass_treble(s_eq_type, s_bass, s_treble);
    return 0;
}

int audio_get_eq_type(void)
{
    return s_eq_type;
}

int audio_set_reverb_type(int type)
{
    int idx;

    if (type < 0) type = 0;
    if (type >= AUDIO_REVERB_MAX) type = 0;

    s_reverb_type = type;
    save_config_int("reverb_type", type);

    switch (type) {
    case AUDIO_REVERB_OFF:         idx = 6; break;
    case AUDIO_REVERB_LIVINGROOM:  idx = 0; break;
    case AUDIO_REVERB_HALL:        idx = 1; break;
    case AUDIO_REVERB_CONCERT:     idx = 2; break;
    case AUDIO_REVERB_CAVE:        idx = 3; break;
    case AUDIO_REVERB_BATHROOM:    idx = 4; break;
    case AUDIO_REVERB_ARENA:       idx = 5; break;
    default:                       idx = 6; break;
    }

    return SetReverbType((REVERBTYPE_T)type, gAudSeReverbCoefTab[idx]);
}

int audio_get_reverb_type(void)
{
    return s_reverb_type;
}

const char *audio_eq_type_name(int type)
{
    if (type < 0 || type >= AUDIO_EQ_MAX) return "Unknown";
    return s_eq_names[type];
}

const char *audio_reverb_type_name(int type)
{
    if (type < 0 || type >= AUDIO_REVERB_MAX) return "Unknown";
    return s_reverb_names[type];
}
