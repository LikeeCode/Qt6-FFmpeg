#ifndef VIDEOMASTER_H
#define VIDEOMASTER_H

// Compatibility with C and C99 standards
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

#include <QString>
#include <QDebug>

class VideoMaster
{
private:
    AVFormatContext *input_fmt_ctx;
    AVFormatContext *output_fmt_ctx;

    AVCodecContext *audio_decoding_ctx;
    AVCodecContext *video_decoding_ctx;

    AVCodecContext *audio_encoding_ctx;
    AVCodecContext *video_encoding_ctx;

    AVStream *output_audio_stream;
    AVStream *output_video_stream;

    int input_audio_stream_index = -1;
    int input_video_stream_index = -1;

public:
    int openInputFile(QString filename);
    int openOutputFile(QString filename);

    VideoMaster();
};

#endif // VIDEOMASTER_H
