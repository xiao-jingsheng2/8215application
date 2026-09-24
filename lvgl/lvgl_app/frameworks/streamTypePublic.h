#ifndef STREAM_TYPE_PUBLIC_H
#define STREAM_TYPE_PUBLIC_H

typedef enum {
    AUD_STREAM_TYPE_MIN         = 1,
    AUD_STREAM_TYPE_DEFAULT     = AUD_STREAM_TYPE_MIN,
    AUD_STREAM_TYPE_MM          = 1,
    AUD_STREAM_TYPE_GIS         = 2,
    AUD_STREAM_TYPE_BTHFP       = 3,
    AUD_STREAM_TYPE_BTRING      = 4,
    AUD_STREAM_TYPE_BACKCAR     = 5,
    AUD_STREAM_TYPE_RDS         = 6,
    AUD_STREAM_TYPE_MAX         = 6,
    AUD_STREAM_TYPE_ERROR       = 0xFFFFFFFF,
    AUD_STREAM_TYPE_INVALID     = 0xFFFFFFFF,
} audio_stream_type_t;

#endif
