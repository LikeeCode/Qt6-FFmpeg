#ifndef OVERLAYGENERATOR_H
#define OVERLAYGENERATOR_H

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
#include <libavutil/imgutils.h>
}

#include <QObject>
#include <QString>
#include <QImage>
#include <QStandardPaths>
#include <QPainter>
#include <QDebug>

#include "overlay.h"

class OverlayGenerator : public QObject
{
    Q_OBJECT

private:
    Overlay* overlay;

    int overlayX = 0;
    int overlayY = 0;
    QImage frameImage;

public:
    OverlayGenerator(QObject* parent = nullptr);

    void setOverlayX(int x);
    void setOverlayY(int y);
    void generateOverlayAt(AVFrame *frame, AVCodecContext* codec_ctx, double timestamp = 0.0);

    static QImage avFrameToQImage(AVFrame* frame, AVCodecContext* codec_ctx, float scalingFactor = 1.0f);
    static void qImageToAVFrame(QImage image, AVFrame* frame, AVCodecContext* codec_ctx);
};

#endif // OVERLAYGENERATOR_H
