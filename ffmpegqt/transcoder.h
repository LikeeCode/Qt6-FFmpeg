#ifndef TRANSCODER_H
#define TRANSCODER_H

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
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

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

#define SLIDER_ANIM_DUR 1000 // milliseconds

#include "filteringcontext.h"
#include "streamcontext.h"

#include <QImage>
#include <QStandardPaths>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQmlContext>
#include <QPainter>
#include <QDebug>

class Transcoder
{
private:
    AVFormatContext *ifmt_ctx;
    AVFormatContext *ofmt_ctx;

    FilteringContext *filter_ctx;
    StreamContext *stream_ctx;

    int open_input_file(const char *filename);
    int open_output_file(const char *filename);
    int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
            AVCodecContext *enc_ctx, const char *filter_spec);
    int init_filters(void);
    int encode_write_frame(unsigned int stream_index, int flush);
    int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index);
    int flush_encoder(unsigned int stream_index);

    void create_frame_overlay(AVCodecContext* codec_ctx, AVFrame* frame);
    double get_frame_timestamp(AVCodecContext* codec_ctx, AVFrame* frame);
    void createSliderAnimation();
    QString getNumericValueAt(float timestamp);
    float getShapeValueAt(float timestamp);
    float getSliderValueAt(float timestamp);
    QImage get_overlay_image(float timestamp);
    QImage get_combined_image(QImage* bg, QImage* overlay);
    QImage frame_to_image(AVFrame* frame);
    void image_to_frame(QImage* image, AVFrame *frame);

    uint64_t frames_counter = 0;

    QQmlApplicationEngine* engine;
    QPropertyAnimation sliderAnimation;
    QRandomGenerator randomGenerator;
    QQuickView* view;

    static AVFrame* pFrmDst;
    static SwsContext* img_convert_ctx;

public:
    Transcoder(QQmlApplicationEngine* e);

    int transcode(QString input, QString output);
};

#endif // TRANSCODER_H
