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
#include <libswscale/swscale.h>
}

#include <QString>
#include <QImage>
#include <QStandardPaths>
#include <QDebug>

class VideoMaster
{
private:
    AVFormatContext *input_fmt_ctx = nullptr;
    AVFormatContext *output_fmt_ctx = nullptr;

    AVCodecContext *audio_codec_ctx = nullptr;
    AVCodecContext *video_codec_ctx = nullptr;

    AVStream *input_audio_stream = nullptr;
    AVStream *input_video_stream = nullptr;

    AVStream *output_audio_stream = nullptr;
    AVStream *output_video_stream = nullptr;

    int input_audio_stream_index = -1;
    int input_video_stream_index = -1;

    QImage avFrameToQImage(AVFrame* frame);

    static AVFrame *pFrmDst;
    static SwsContext *img_convert_ctx;

public:
    int open_input_file(const char *filename);
    int open_output_file(const char *filename);

    VideoMaster();

    int generateOverlayVideo(QString input, QString output);
};

#endif // VIDEOMASTER_H
