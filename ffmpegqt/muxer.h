#ifndef MUXER_H
#define MUXER_H

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

#include <QImage>
#include <QDebug>

#include "outputstream.h"

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC

class Muxer
{
private:
    uint8_t *buffer;                    // Store decoded picture buffer
    AVPacket *avPacket;                 // Package object
    AVFrame *avFrame;                   // Frame object
    AVFormatContext *avFormatContext;   // Format object
    AVCodecContext *videoCodec;         // Video decoder
    AVCodecContext *audioCodec;         // Audio decoder
    SwsContext *swsContext;             // Process image data objects

    AVDictionary *options;              // Parameter object
    AVCodec *videoDecoder;              // Video decoding
    AVCodec *audioDecoder;              // Audio decoding

    FILE *file;

    void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                    AVStream *st, AVFrame *frame, AVPacket *pkt);
    void add_stream(OutputStream *ost, AVFormatContext *oc,
                    const AVCodec **codec,
                    enum AVCodecID codec_id);
    AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                      uint64_t channel_layout,
                                      int sample_rate, int nb_samples);
    void open_audio(AVFormatContext *oc, const AVCodec *codec,
                           OutputStream *ost, AVDictionary *opt_arg);
    AVFrame *get_audio_frame(OutputStream *ost);
    int write_audio_frame(AVFormatContext *oc, OutputStream *ost);

    AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
    void open_video(AVFormatContext *oc, const AVCodec *codec,
                    OutputStream *ost, AVDictionary *opt_arg);
    void fill_yuv_image(AVFrame *pict, int frame_index, int width, int height);
    AVFrame *get_video_frame(OutputStream *ost);
    int write_video_frame(AVFormatContext *oc, OutputStream *ost);
    void close_stream(AVFormatContext *oc, OutputStream *ost);

public:
    Muxer();

    int createVideo(QMap<QString, QString> args);
    static AVFrame* QImagetoAVFrame(QImage qImage);
    static QImage AVFrametoQImage(AVFrame* avFrame);
};

#endif // MUXER_H