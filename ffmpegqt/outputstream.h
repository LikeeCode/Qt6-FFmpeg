#ifndef OUTPUTSTREAM_H
#define OUTPUTSTREAM_H

#include <libavformat/avformat.h>

struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;
    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;
    AVFrame *frame;
    AVFrame *tmp_frame;
    float t, tincr, tincr2;
    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
};

#endif // OUTPUTSTREAM_H
