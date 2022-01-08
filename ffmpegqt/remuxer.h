#ifndef REMUXER_H
#define REMUXER_H

// Compatibility with C and C99 standards
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include "libavutil/imgutils.h"

#ifdef ffmpegdevice
#include "libavdevice/avdevice.h"
#endif

#ifndef gcc45
#include "libavutil/hwcontext.h"
#include "libavutil/hwcontext_qsv.h"
#endif
}

class Remuxer
{
public:
    Remuxer();
};

#endif // REMUXER_H
