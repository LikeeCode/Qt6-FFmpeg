#include "overlaygenerator.h"

AVFrame* OverlayGenerator::pFrmDst;
SwsContext* OverlayGenerator::img_convert_ctx;

OverlayGenerator::OverlayGenerator(QQmlApplicationEngine *e) : engine(e)
{
    createSliderAnimation();
}

OverlayGenerator::~OverlayGenerator()
{
    av_frame_free(&pFrmDst);
    sws_freeContext(img_convert_ctx);
}

QString OverlayGenerator::getNumericValueAt(float timestamp)
{
    static int lastTimestamp = 0;
    static int lastValue = 0;

    int currentTimestamp = static_cast<int>(timestamp / 1000);
    if(currentTimestamp != lastTimestamp){
        lastTimestamp = currentTimestamp;
        lastValue = randomGenerator.bounded(99, 999);
    }

    return QString::number(lastValue);
}

float OverlayGenerator::getShapeValueAt(float timestamp)
{
    static int lastTimestamp = 0;
    static float lastValue = 0.0f;

    int currentTimestamp = static_cast<int>(timestamp / 300);
    if(currentTimestamp != lastTimestamp){
        lastTimestamp = currentTimestamp;
        lastValue = randomGenerator.bounded(1.0);
    }

    return lastValue;
}

float OverlayGenerator::getSliderValueAt(float timestamp)
{
    int timestampInMs = timestamp * 1000;
    int currentTime = timestampInMs % SLIDER_ANIM_DUR;
    sliderAnimation.setCurrentTime(currentTime);
    return sliderAnimation.currentValue().toFloat();
}

void OverlayGenerator::createSliderAnimation()
{
    sliderAnimation.setDuration(SLIDER_ANIM_DUR);
    sliderAnimation.setKeyValueAt(0.0, 0.0);
    sliderAnimation.setKeyValueAt(0.5, 1.0);
    sliderAnimation.setKeyValueAt(1.0, 0.0);
}

void OverlayGenerator::generateOverlayAt(AVFrame *frame, double timestamp, int x, int y)
{
    // Generate frame image
    QImage frameImage = avFrameToQImage(frame);

    // Generate overlay image
    auto numericValue = getNumericValueAt(timestamp);
    auto shapeValue = getShapeValueAt(timestamp);
    auto sliderValue = getSliderValueAt(timestamp);

    view = new QQuickView(engine, nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/qml/Overlay.qml")));
    view->setColor(QColorConstants::Transparent);
    view->rootContext()->setContextProperty("OVERLAY_NUMERIC", numericValue);
    view->rootContext()->setContextProperty("OVERLAY_SHAPE", shapeValue);
    view->rootContext()->setContextProperty("OVERLAY_SLIDER", sliderValue);

    QImage overlayImage = view->grabWindow();

    // Combine frame with overlay
    QPainter painter(&frameImage);
    painter.drawImage(x, y, overlayImage);
    painter.end();

    QString imgName = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) + "/img_" +
                QStringLiteral("%1").arg(frameCounter, 5, 10, QLatin1Char('0')) +
                ".png";
    frameImage.save(imgName);
    frameCounter++;
}

QImage OverlayGenerator::avFrameToQImage(AVFrame* frame)
{
    if(pFrmDst == nullptr){
        pFrmDst = av_frame_alloc();
    }

    if (img_convert_ctx == nullptr){
        img_convert_ctx =
                sws_getContext(frame->width, frame->height,
                               (AVPixelFormat)frame->format, frame->width, frame->height,
                               AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

        pFrmDst->format = AV_PIX_FMT_RGB24;
        pFrmDst->width  = frame->width;
        pFrmDst->height = frame->height;

        if (av_frame_get_buffer(pFrmDst, 0) < 0){
            return QImage();
        }
    }

    if (img_convert_ctx == nullptr){
        return QImage();
    }

    sws_scale(img_convert_ctx, (const uint8_t *const *)frame->data,
              frame->linesize, 0, frame->height, pFrmDst->data,
              pFrmDst->linesize);

    QImage img(pFrmDst->width, pFrmDst->height, QImage::Format_RGB888);

    for(int y=0; y<pFrmDst->height; ++y){
        memcpy(img.scanLine(y), pFrmDst->data[0]+y*pFrmDst->linesize[0], pFrmDst->linesize[0]);
    }

    return img;
}

void OverlayGenerator::QImageToAVFrame(QImage image, AVFrame* frame){
    if((frame->width == image.width()) && (frame->height == image.height())){
//        frame->width = image.width();
//        frame->height = image.height();
//        frame->format = AV_PIX_FMT_ARGB;
//        frame->linesize[0] = image.width();

        av_image_fill_arrays(frame->data, frame->linesize, (uint8_t*)image.bits(),
                           AV_PIX_FMT_ARGB, frame->width, frame->height, 1);
    }
}
