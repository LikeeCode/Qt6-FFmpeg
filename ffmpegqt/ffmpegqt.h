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

class FFmpegQt
{
private:
    uint8_t *buffer;                    // Store decoded picture buffer
    AVPacket *avPacket;                 // Package object
    AVFrame *avFrame;                   // Frame object
    AVFrame *avFrame2;                  // Frame object
    AVFrame *avFrame3;                  // Frame object
    AVFormatContext *avFormatContext;   // Format object
    AVCodecContext *videoCodec;         // Video decoder
    AVCodecContext *audioCodec;         // Audio decoder
    SwsContext *swsContext;             // Process image data objects

    AVDictionary *options;              // Parameter object
    AVCodec *videoDecoder;              // Video decoding
    AVCodec *audioDecoder;              // Audio decoding

    int frame_count, video_outbuf_size;

public:
    FFmpegQt();

    AVFrame* QImagetoAVFrame(QImage qImage);
    QImage AVFrametoQImage(AVFrame* avFrame);
};

#endif // FFMPEGQT_H
