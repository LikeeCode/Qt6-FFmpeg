#include "ffmpegqt.h"

FFmpegQt::FFmpegQt()
{
}

AVFrame* FFmpegQt::QImagetoAVFrame(QImage qImage){
    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = qImage.width();
    avFrame->height = qImage.height();
    avFrame->format = AV_PIX_FMT_ARGB;
    avFrame->linesize[0] = qImage.width();

    av_image_fill_arrays(avFrame->data, avFrame->linesize, (uint8_t*)qImage.bits(),
                       AV_PIX_FMT_ARGB, avFrame->width, avFrame->height, 1);

    return avFrame;
}

QImage FFmpegQt::AVFrametoQImage(AVFrame* avFrame){
    QImage qImage(avFrame->width, avFrame->height, QImage::Format_RGB32);
    int* src = (int*)avFrame->data[0];

    for (int y = 0; y < avFrame->height; y++) {
       for (int x = 0; x < avFrame->width; x++) {
          qImage.setPixel(x, y, src[x] & 0x00ffffff);
       }
       src += avFrame->width;
    }

    return qImage;
}
