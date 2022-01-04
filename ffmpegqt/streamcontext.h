#ifndef STREAMCONTEXT_H
#define STREAMCONTEXT_H

extern "C" {
#include <libavformat/avformat.h>
}

struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;

    AVFrame *dec_frame;
};

#endif // STREAMCONTEXT_H
