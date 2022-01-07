#include "overlaygenerator.h"

SwsContext* OverlayGenerator::sws_ctx;

OverlayGenerator::OverlayGenerator(QQmlApplicationEngine *e) : engine(e)
{
    createSliderAnimation();
}

OverlayGenerator::~OverlayGenerator()
{
    sws_freeContext(sws_ctx);
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

void OverlayGenerator::generateOverlayAt(AVFrame *frame, AVCodecContext* codec_ctx,
                                         double timestamp, int x, int y)
{
    // Generate frame image
    QImage frameImage = avFrameToQImage(frame, codec_ctx);

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

QImage OverlayGenerator::avFrameToQImage(AVFrame* frame, AVCodecContext* codec_ctx)
{
    if(!sws_ctx){
        SwsContext* sws_ctx = sws_getContext(frame->width, frame->height, codec_ctx->pix_fmt,
                                             frame->width, frame->height, AV_PIX_FMT_RGB0,
                                             SWS_BICUBIC, NULL, NULL, NULL);

        if(!sws_ctx){
            qDebug() << "Could not initialize SwsContext";
            return QImage();
        }
    }

    uint8_t* src_data = new uint8_t[frame->width * frame->height * 4];
    uint8_t* dest_data[4] = { src_data, NULL, NULL, NULL };
    int dest_linesize[4] = {frame->width * 4, 0, 0, 0};

    sws_scale(sws_ctx, frame->data, frame->linesize,
              0, frame->height, // start from horizontal zero to the height
              dest_data, dest_linesize);

    QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
    for(int y = 0; y < frame->height; ++y){
        memcpy(image.scanLine(y),
               dest_data[0] + y * dest_linesize[0],
               dest_linesize[0]);
    }

    sws_freeContext(sws_ctx);

    return image;
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
