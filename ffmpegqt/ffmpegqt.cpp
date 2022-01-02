#include "ffmpegqt.h"

FFmpegQt::FFmpegQt()
{
}

void FFmpegQt::ffmpeg_encoder_init_frame(AVFrame **framep, int width, int height) {
    int ret;
    AVFrame *frame;
    frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "Could not allocate video frame";
        exit(1);
    }
    frame->format = videoCodec->pix_fmt;
    frame->width  = width;
    frame->height = height;
    ret = av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, AV_PIX_FMT_ARGB, 32);
    if (ret < 0) {
        qDebug() << "Could not allocate raw picture buffer";
        exit(1);
    }
    *framep = frame;
}

void FFmpegQt::ffmpeg_encoder_start(const char *filename, AVCodecID codecID, int fps, int width, int height) {
    AVCodec *codec;

    codec = avcodec_find_encoder(codecID);
    if (!codec) {
        qDebug() << "Codec not found";
        exit(1);
    }

    videoCodec = avcodec_alloc_context3(codec);
    if (!videoCodec) {
        qDebug() << "Could not allocate video codec context";
        exit(1);
    }

    videoCodec->bit_rate = 400000;
    videoCodec->width = width;
    videoCodec->height = height;
    videoCodec->time_base.num = 1;
    videoCodec->time_base.den = fps;
    videoCodec->gop_size = 10;
    videoCodec->max_b_frames = 1;
    videoCodec->pix_fmt = AV_PIX_FMT_YUV420P;

    if (codecID == AV_CODEC_ID_H264)
        av_opt_set(videoCodec->priv_data, "preset", "slow", 0);

    if (avcodec_open2(videoCodec, codec, NULL) < 0) {
        qDebug() << "Could not open codec";
        exit(1);
    }

    file = fopen(filename, "wb");
    if (!file) {
        qDebug() << "Could not open" << " " << filename;
        exit(1);
    }

    ffmpeg_encoder_init_frame(&avFrame, width, height);
}

void FFmpegQt::ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb) {
    const int in_linesize[1] = { 3 * avFrame->width };
    swsContext = sws_getCachedContext(swsContext,
            avFrame->width, avFrame->height, AV_PIX_FMT_RGB24,
            avFrame->width, avFrame->height, AV_PIX_FMT_YUV420P,
            0, NULL, NULL, NULL);
    sws_scale(swsContext, (const uint8_t * const *)&rgb, in_linesize, 0,
            avFrame->height, avFrame->data, avFrame->linesize);
}

void FFmpegQt::ffmpeg_encoder_finish(void) {
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    int got_output, ret;

    do {
        fflush(stdout);
        ret = avcodec_encode_video2(videoCodec, avPacket, NULL, &got_output);

        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }

        if (got_output) {
            fwrite(avPacket->data, 1, avPacket->size, file);
            av_packet_unref(avPacket);
        }
    } while (got_output);

    fwrite(endcode, 1, sizeof(endcode), file);
    fclose(file);

    avcodec_close(videoCodec);

    av_free(videoCodec);
    av_freep(&avFrame->data[0]);
    av_frame_free(&avFrame);
}

void FFmpegQt::ffmpeg_encoder_encode_frame(uint8_t *rgb) {
    Q_UNUSED(rgb)

    int ret, got_output;

    ffmpeg_encoder_set_frame_yuv_from_rgb(rgb);

    av_init_packet(avPacket);
    avPacket->data = NULL;
    avPacket->size = 0;

    ret = avcodec_encode_video2(videoCodec, avPacket, avFrame, &got_output);

    if (ret < 0) {
        fprintf(stderr, "Error encoding frame\n");
        exit(1);
    }

    if (got_output) {
        fwrite(avPacket->data, 1, avPacket->size, file);
        av_packet_unref(avPacket);
    }
}

void FFmpegQt::encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
    int ret;
    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3" PRId64"\n", frame->pts);
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        qDebug() << "Error sending a frame for encoding";
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            qDebug() << "Error during encoding";
            exit(1);
        }
        printf("Write packet %3" PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

void FFmpegQt::ffmpeg_encoder_encode_video(QString fileName, int width, int height, int numFrames)
{
    AVFrame *frame = av_frame_alloc();
    av_image_alloc(frame->data, frame->linesize, width, height, AV_PIX_FMT_ARGB, 1);
    frame->width = width;
    frame->height = height;
    frame->format = AV_PIX_FMT_ARGB;
    frame->linesize[0] = width;

    ffmpeg_encoder_start(fileName.toStdString().c_str(), AV_CODEC_ID_H264, 25, width, height);

    for(int i = 0; i < numFrames; ++i){
        QImage img;

        av_image_fill_arrays(frame->data, frame->linesize, (uint8_t*)img.bits(),
        AV_PIX_FMT_ARGB, width, height, 1);
    }

    ffmpeg_encoder_finish();
}

void FFmpegQt::createVideo(QString fileName, QString codecName)
{
    const char *filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    filename = fileName.toLocal8Bit().data();
    codec_name = codecName.toLocal8Bit().data();

    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        qDebug() << "Codec " << codec_name << " not found";
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        qDebug() << "Could not allocate video codec context";
        exit(1);
    }
    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);
    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    c->time_base = (AVRational){1, 25};
    c->framerate = (AVRational){25, 1};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        qDebug() << "Could not open codec";
        exit(1);
    }
    f = fopen(filename, "wb");
    if (!f) {
        qDebug() << "Could not open " << filename;
        exit(1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        qDebug() << "Could not allocate video frame";
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        qDebug() << "Could not allocate the video frame data";
        exit(1);
    }
    /* encode 1 second of video */
    for (i = 0; i < 25; i++) {
        fflush(stdout);
        /* make sure the frame data is writable */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            exit(1);
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
        /* Cb and Cr */
        for (y = 0; y < c->height/2; y++) {
            for (x = 0; x < c->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }
        frame->pts = i;
        /* encode the image */
        encode(c, frame, pkt, f);
    }
    /* flush the encoder */
    encode(c, NULL, pkt, f);
    /* add sequence end code to have a real MPEG file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
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
