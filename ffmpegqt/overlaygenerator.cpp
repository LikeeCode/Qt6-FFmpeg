#include "overlaygenerator.h"

SwsContext* OverlayGenerator::sws_ctx = NULL;

OverlayGenerator::OverlayGenerator(QQmlApplicationEngine *e) : engine(e)
{
    createSliderAnimation();
}

OverlayGenerator::~OverlayGenerator()
{
    if(sws_ctx){
        sws_freeContext(sws_ctx);
    }
}

QString OverlayGenerator::getNumericValueAt(float timestamp)
{
    int currentTimestamp = timestamp * 1000 / 1000;
    if(currentTimestamp!= lastTimestampForNumeric){
        lastTimestampForNumeric = currentTimestamp;
        lastValueForNumeric = randomGenerator.bounded(99, 999);
    }

    return QString::number(lastValueForNumeric);
}

float OverlayGenerator::getShapeValueAt(float timestamp)
{
    int currentTimestamp = timestamp * 1000 / 300;
    if(currentTimestamp != lastTimestampForShape){
        lastTimestampForShape = currentTimestamp;
        lastValueForShape = (float)randomGenerator.generateDouble();
    }

    return lastValueForShape;
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
    QImage frameImage(view->width(), view->height(), QImage::Format_RGBA8888);
    frameImage.fill(QColorConstants::Transparent);
    QPainter painter(&frameImage);
    painter.drawImage(0, 0, overlayImage);
    painter.end();

    QImageToAVFrame(frameImage, frame, x, y);
}

QImage OverlayGenerator::avFrameToQImage(AVFrame* frame, AVCodecContext* codec_ctx)
{
    if(!sws_ctx){
        sws_ctx = sws_getContext(frame->width, frame->height, codec_ctx->pix_fmt,
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

    return image;
}

void OverlayGenerator::QImageToAVFrame(QImage image, AVFrame* frame, int offset_x, int offset_y)
{
    QColor color;
    int r, g, b, a;
    int Y, U, V;

    int w_max = ((image.width() + offset_x) < frame->width) ? (image.width() + offset_x) : frame->width;
    int h_max = ((image.height() + offset_y) < frame->height) ? (image.height() + offset_y) : frame->height;

    for (int h = offset_y; h < h_max; h++)
    {
        for (int w = offset_x; w < w_max; w++)
        {
            color = image.pixelColor(w - offset_x, h - offset_y);

            r = color.red();
            g = color.green();
            b = color.blue();
            a = color.alpha();

            if(a > 0){
                RGBtoYUV(r, g, b, Y, U, V);

                frame->data[0][h * frame->linesize[0] + w] = (uchar)Y;

                if(h % 2 == 0 && w % 2 == 0)
                {
                    frame->data[1][h/2 * (frame->linesize[1]) + w/2] = (uchar)U;
                    frame->data[2][h/2 * (frame->linesize[2]) + w/2] = (uchar)V;
                }
            }
        }
    }
}

void OverlayGenerator::RGBtoYUV(const int R, const int G, const int B, int& Y, int& U, int& V)
{
    Y = ((  66 * R + 129 * G +  25 * B) >> 8) +  16;
    U = ((- 38 * R + -74 * G + 112 * B) >> 8) + 128;
    V = (( 112 * R + -94 * G + -18 * B) >> 8) + 128;
}

void OverlayGenerator::RGBtoYUV(const double R, const double G, const double B, double& Y, double& U, double& V)
{
    Y =  0.257 * R + 0.504 * G + 0.098 * B +  16;
    U = -0.148 * R - 0.291 * G + 0.439 * B + 128;
    V =  0.439 * R - 0.368 * G - 0.071 * B + 128;
}

void OverlayGenerator::YUVtoRGB(int Y, int U, int V, int& R, int& G, int& B)
{
    R = (Y + 1.4075 * (V - 128));
    G = (Y - 0.3455 * (U - 128) - (0.7169 * (V - 128)));
    B = (Y + 1.7790 * (U - 128));
}

void OverlayGenerator::YUVtoRGB(double Y, double U, double V, double& R, double& G, double& B)
{
    Y -= 16;
    U -= 128;
    V -= 128;
    R = 1.164 * Y             + 1.596 * V;
    G = 1.164 * Y - 0.392 * U - 0.813 * V;
    B = 1.164 * Y + 2.017 * U;
}
