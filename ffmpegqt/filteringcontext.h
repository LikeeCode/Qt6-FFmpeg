#ifndef FILTERINGCONTEXT_H
#define FILTERINGCONTEXT_H

#include <libavcodec/avcodec.h>
#include <libavfilter/buffersink.h>

struct FilteringContext {
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFilterGraph *filter_graph;

    AVPacket *enc_pkt;
    AVFrame *filtered_frame;
};

#endif // FILTERINGCONTEXT_H
