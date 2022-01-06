#include "videomaster.h"

AVFrame* VideoMaster::pFrmDst;
SwsContext* VideoMaster::img_convert_ctx;

VideoMaster::VideoMaster()
{

}

int VideoMaster::generateOverlayVideo(QString input, QString output)
{
    int ret = -1;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if ((ret = open_input_file(input.toLocal8Bit().data())) < 0){
        goto end;
    }

    if ((ret = open_output_file(output.toLocal8Bit().data())) < 0){
        goto end;
    }

    // Read all packets
    while(av_read_frame(input_fmt_ctx, packet) >= 0){
        // Process audio stream
        if(packet->stream_index == input_audio_stream_index){
//            av_packet_rescale_ts(packet,
//                                 input_fmt_ctx->streams[input_audio_stream_index]->time_base,
//                                 audio_codec_ctx->time_base);

            ret = avcodec_send_packet(audio_codec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Decoding audio packet failed\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(audio_codec_ctx, frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
                    break;
                }
                else if (ret < 0){
                    goto end;
                }
            }
        }

        // Process video stream
        if(packet->stream_index == input_video_stream_index){
            // Get frame timestamp
            double pts = 0.0;
            if(packet->dts != AV_NOPTS_VALUE) {
                  pts = packet->dts;
            }
            pts *= av_q2d(input_fmt_ctx->streams[input_video_stream_index]->time_base);
            qDebug() << "Timestamp: " << pts;

//            av_packet_rescale_ts(packet,
//                                 input_fmt_ctx->streams[input_video_stream_index]->time_base,
//                                 video_codec_ctx->time_base);

            ret = avcodec_send_packet(video_codec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Decoding video packet failed\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(video_codec_ctx, frame);
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
                    break;
                }
                else if (ret < 0){
                    goto end;
                }

                int framesCounter = 0;
                QImage img = avFrameToQImage(frame);
                QString imgName = QStandardPaths::writableLocation(
                            QStandardPaths::StandardLocation::DocumentsLocation) + "/img_" +
                            QStringLiteral("%1").arg(framesCounter, 5, 10, QLatin1Char('0')) +
                            ".png";
                img.save(imgName);
            }
        }
    }

end:

    avformat_close_input(&input_fmt_ctx);
    avformat_free_context(input_fmt_ctx);

    av_frame_free(&pFrmDst);

    return ret;
}

QImage VideoMaster::avFrameToQImage(AVFrame* frame)
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

int VideoMaster::open_input_file(const char *filename)
{
    int ret = -1;

    input_fmt_ctx = NULL;
    if ((ret = avformat_open_input(&input_fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(input_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    for (unsigned int i = 0; i < input_fmt_ctx->nb_streams; ++i) {
        AVStream *stream = input_fmt_ctx->streams[i];
        AVCodecContext *codec_ctx;

        const AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }

        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }

        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                   "for stream #%u\n", i);
            return ret;
        }

        // Get the first audio stream
        if((codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) && (input_audio_stream_index == -1)){
            codec_ctx->framerate = av_guess_frame_rate(input_fmt_ctx, stream, NULL);

            // Open decoder
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }

            audio_codec_ctx = codec_ctx;
            input_audio_stream = stream;
            input_audio_stream_index = i;
        }
        // Get the first video stream
        else if((codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) && (input_video_stream_index == -1)){
            codec_ctx->framerate = av_guess_frame_rate(input_fmt_ctx, stream, NULL);

            // Open decoder
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }

            video_codec_ctx = codec_ctx;
            input_video_stream = stream;
            input_video_stream_index = i;
        }

        if((input_audio_stream_index != -1) && (input_video_stream_index != -1)){
            break;
        }
    }

    av_dump_format(input_fmt_ctx, 0, filename, 0);

    return 0;
}

int VideoMaster::open_output_file(const char *filename)
{
    int ret = -1;
    const AVCodec *audio_encoder, *video_encoder;

    output_fmt_ctx = NULL;
    avformat_alloc_output_context2(&output_fmt_ctx, NULL, NULL, filename);
    if (!output_fmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }

    if(input_audio_stream_index != -1){
        output_audio_stream = avformat_new_stream(output_fmt_ctx, NULL);
        if (!output_audio_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output audio stream\n");
            return AVERROR_UNKNOWN;
        }

        audio_encoder = avcodec_find_encoder(audio_codec_ctx->codec_id);

        if (!audio_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary audio encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        ret = avcodec_open2(audio_codec_ctx, audio_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_audio_stream->codecpar, audio_codec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output audio stream");
            return ret;
        }

        output_audio_stream->time_base = audio_codec_ctx->time_base;
    }

    if(input_video_stream_index != -1){
        output_video_stream = avformat_new_stream(output_fmt_ctx, NULL);
        if (!output_video_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output video stream\n");
            return AVERROR_UNKNOWN;
        }

        video_encoder = avcodec_find_encoder(video_codec_ctx->codec_id);

        if (!video_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary video encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        ret = avcodec_open2(video_codec_ctx, video_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_video_stream->codecpar, video_codec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output video stream");
            return ret;
        }

        output_video_stream->time_base = video_codec_ctx->time_base;
    }

    av_dump_format(output_fmt_ctx, 0, filename, 1);

    if (!(output_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }

    // Init muxer, write output file header
    ret = avformat_write_header(output_fmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}
