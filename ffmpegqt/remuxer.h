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
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <QString>
#include <QDebug>

class Remuxer
{
private:
    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag);

public:
    Remuxer();

    int remux(QString input, QString output);
};

#endif // REMUXER_H
