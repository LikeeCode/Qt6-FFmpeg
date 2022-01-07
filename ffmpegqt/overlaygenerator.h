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
#include <QQuickView>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPainter>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QDebug>

#define SLIDER_ANIM_DUR 1000 // milliseconds

class OverlayGenerator
{
private:
    QQmlApplicationEngine *engine;
    QQuickView* view;
    QRandomGenerator randomGenerator;
    QPropertyAnimation sliderAnimation;
    int lastTimestampForNumeric = 0;
    int lastTimestampForShape = 0;
    int lastValueForNumeric = 0;
    float lastValueForShape = 0.0f;

    static SwsContext *sws_ctx;
    int frameCounter = 0;

    QString getNumericValueAt(float timestamp);
    float getShapeValueAt(float timestamp);
    float getSliderValueAt(float timestamp);
    void createSliderAnimation();

public:
    OverlayGenerator(QQmlApplicationEngine *e);
    ~OverlayGenerator();

    void generateOverlayAt(AVFrame *frame, AVCodecContext* codec_ctx,
                           double timestamp = 0.0, int x = 0, int y = 0);

    static QImage avFrameToQImage(AVFrame* frame, AVCodecContext* codec_ctx);
    static void QImageToAVFrame(QImage image, AVFrame* frame,
                                int offset_x = 0, int offset_y = 0);

    static void RGBtoYUV(const int R, const int G, const int B, int& Y, int& U, int& V);
    static void RGBtoYUV(const double R, const double G, const double B, double& Y, double& U, double& V);
    static void YUVtoRGB(int Y, int U, int V, int& R, int& G, int& B);
    static void YUVtoRGB(double Y, double U, double V, double& R, double& G, double& B);
};

#endif // OVERLAYGENERATOR_H
