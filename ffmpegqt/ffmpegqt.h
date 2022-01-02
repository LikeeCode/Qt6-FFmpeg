#ifndef FFMPEGQT_H
#define FFMPEGQT_H

// Compatibility with C and C99 standards
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C" {
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libavutil/frame.h"
#include "libavutil/pixdesc.h"
#include "libavutil/avassert.h"
#include "libavutil/imgutils.h"
#include "libavutil/ffversion.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"

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

class FFmpegQt
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

    int frame_count, video_outbuf_size;

    void ffmpeg_encoder_init_frame(AVFrame **framep, int width, int height);
    void ffmpeg_encoder_start(const char *filename, AVCodecID codecID, int fps, int width, int height);
    void ffmpeg_encoder_finish(void);
    void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb);
    void ffmpeg_encoder_encode_frame(uint8_t *rgb);

    void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile);

public:
    FFmpegQt();

    void ffmpeg_encoder_encode_video(QString fileName, int width, int height, int numFrames);

    void createVideo(QString fileName, QString codecName);
    static AVFrame* QImagetoAVFrame(QImage qImage);
    static QImage AVFrametoQImage(AVFrame* avFrame);
};

#endif // FFMPEGQT_H
