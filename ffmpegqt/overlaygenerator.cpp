#include "overlaygenerator.h"

OverlayGenerator::OverlayGenerator(QObject* parent) : QObject(parent)
{
    overlay = new Overlay;
}

void OverlayGenerator::setOverlayX(int x)
{
    overlayX = x;
}

void OverlayGenerator::setOverlayY(int y)
{
    overlayY = y;
}

void OverlayGenerator::generateOverlayAt(AVFrame *frame, AVCodecContext* codec_ctx, double timestamp)
{
    // Create overlay image
    QImage overlayImage = overlay->getImageAtTimestamp(timestamp);

    // Create frame image
    frameImage = avFrameToQImage(frame, codec_ctx);
//    frameImage = QImage(frame->width, frame->height, QImage::Format_RGBA8888);
//    frameImage.fill(QColorConstants::Blue);

    // Draw overlay over frame image
    QPainter painter(&frameImage);
    painter.drawImage(overlayX, overlayY, overlayImage);

    // Convert overlayed image into frame
    qImageToAVFrame(frameImage, frame, codec_ctx);

//    QImage convertedFrame = avFrameToQImage(frame, codec_ctx);
//    QString name = QStandardPaths::writableLocation(
//                QStandardPaths::StandardLocation::DocumentsLocation) +
//                "/converted_frame.png";
//    convertedFrame.save(name);
}

QImage OverlayGenerator::avFrameToQImage(AVFrame* frame, AVCodecContext* codec_ctx,
                                         float scalingFactor)
{
    SwsContext *swsCtx = sws_getContext(
                frame->width, frame->height,
                codec_ctx->pix_fmt,
                frame->width * scalingFactor,
                frame->height * scalingFactor,
                AV_PIX_FMT_RGB0,
                SWS_BICUBIC, NULL, NULL, NULL);

    uint8_t* src_data = new uint8_t[frame->width * frame->height * 4];
    uint8_t* dest_data[4] = { src_data, NULL, NULL, NULL };
    int dest_linesize[4] = {frame->width * 4, 0, 0, 0};

    sws_scale(swsCtx, frame->data, frame->linesize,
              0, frame->height, // start from horizontal zero to the height
              dest_data, dest_linesize);

    QImage image(frame->width, frame->height, QImage::Format_RGBA8888);
    for(int y = 0; y < frame->height; ++y){
        memcpy(image.scanLine(y),
               dest_data[0] + y * dest_linesize[0],
               dest_linesize[0]);
    }

    delete[] src_data;
    sws_freeContext(swsCtx);

    return image;
}

void OverlayGenerator::qImageToAVFrame(QImage image, AVFrame* frame, AVCodecContext* codec_ctx)
{
    // Prepare SWS Context
    SwsContext *swsCtx = sws_getContext(
                image.width(), image.height(), AV_PIX_FMT_RGB0,
                frame->width, frame->height, codec_ctx->pix_fmt,
                SWS_BICUBIC, NULL, NULL, NULL);

    // Preparing the buffer to get RGBA data
    uint8_t* bits = (uint8_t*)image.bits();
    uint8_t* rgb_data[4] = { bits, 0, 0, 0 };
    int rgb_linesize[4] = { (int)image.bytesPerLine(), 0, 0, 0 };

    // Make pixel format conversion
    sws_scale(swsCtx,
              rgb_data, rgb_linesize,
              0, image.height(),
              frame->data, frame->linesize);

    sws_freeContext(swsCtx);
}
