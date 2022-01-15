#include "ffmpeg.h"

FFmpeg::FFmpeg() : QObject()
{
}

void FFmpeg::setOverlayGenerator(OverlayGenerator *generator)
{
    overlayGenerator = generator;
}

int FFmpeg::openInputFile(const char *filename)
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
            continue;
        }

        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            continue;
        }

        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                   "for stream #%u\n", i);
            continue;
        }

        // Get the first audio stream
        if((codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) && (input_audio_stream_index == -1)){
            // Open decoder
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }

            input_audio_stream_index = i;
            audio_decodec_ctx = codec_ctx;
            nb_audio_frames = stream->nb_frames;
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

            input_video_stream_index = i;
            video_decodec_ctx = codec_ctx;
            nb_video_frames = stream->nb_frames;
        }

        if((input_audio_stream_index != -1) && (input_video_stream_index != -1)){
            break;
        }
    }

    qDebug() << "Number of audio frames: " << nb_audio_frames;
    qDebug() << "Number of video frames: " << nb_video_frames;

    av_dump_format(input_fmt_ctx, 0, filename, 0);

    return 0;
}

int FFmpeg::openOutputFile(const char *filename)
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
        // Add audio stream
        output_audio_stream = avformat_new_stream(output_fmt_ctx, NULL);
        if (!output_audio_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output audio stream\n");
            return AVERROR_UNKNOWN;
        }

        // Find encoder corresponding to codec_id
        audio_encoder = avcodec_find_encoder(audio_decodec_ctx->codec_id);
        if (!audio_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary audio encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        // Allocate encoder context and assign its parameters
        audio_encodec_ctx = avcodec_alloc_context3(audio_encoder);
        audio_encodec_ctx->sample_rate = audio_decodec_ctx->sample_rate;
        audio_encodec_ctx->channel_layout = audio_decodec_ctx->channel_layout;
        audio_encodec_ctx->channels = audio_decodec_ctx->channels;
        audio_encodec_ctx->sample_fmt = audio_decodec_ctx->sample_fmt;
        audio_encodec_ctx->bit_rate = audio_decodec_ctx->bit_rate;
        audio_encodec_ctx->time_base = (AVRational){1, audio_encodec_ctx->sample_rate};

        if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            audio_encodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(audio_encodec_ctx, audio_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_audio_stream->codecpar, audio_encodec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output audio stream");
            return ret;
        }

        output_audio_stream->time_base = av_inv_q(audio_decodec_ctx->framerate);
    }

    if(input_video_stream_index != -1){
        // Add video stream
        output_video_stream = avformat_new_stream(output_fmt_ctx, NULL);
        if (!output_video_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output video stream\n");
            return AVERROR_UNKNOWN;
        }

        // Find encoder corresponding to codec_id
        video_encoder = avcodec_find_encoder(video_decodec_ctx->codec_id);
        if (!video_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary video encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        // Allocate encoder context and assign its parameters
        video_encodec_ctx = avcodec_alloc_context3(video_encoder);
        video_encodec_ctx->width = video_decodec_ctx->width;
        video_encodec_ctx->height = video_decodec_ctx->height;
        video_encodec_ctx->sample_aspect_ratio = video_decodec_ctx->sample_aspect_ratio;
        video_encodec_ctx->pix_fmt = video_encoder->pix_fmts ? video_encoder->pix_fmts[0] : video_decodec_ctx->pix_fmt;
        video_encodec_ctx->time_base = video_decodec_ctx->time_base;

        if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            video_encodec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        ret = avcodec_open2(video_encodec_ctx, video_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_video_stream->codecpar, video_encodec_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output video stream");
            return ret;
        }

        output_video_stream->time_base = av_inv_q(video_decodec_ctx->framerate);
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

int FFmpeg::writeFrame(AVFormatContext *fmt_ctx, AVCodecContext *codec_ctx,
                             AVStream *stream, AVFrame* frame, AVPacket *packet)
{
    int ret = -1;

    av_packet_unref(packet);

    // Send the frame to the encoder
    ret = avcodec_send_frame(codec_ctx, frame);
    if (ret < 0) {
        qDebug() << "Error sending a frame to the encoder: " << ret;
        exit(1);
    }

    while (ret >= 0) {
        if(cancelled){
            break;
        }

        ret = avcodec_receive_packet(codec_ctx, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            qDebug() << "Error encoding a frame: " << ret;
            exit(1);
        }

        double pts = 0.0;
        if(packet->dts != AV_NOPTS_VALUE) {
              pts = packet->dts;
        }
        pts *= av_q2d(stream->time_base);

        // Rescale output packet timestamp values from codec to stream timebase
        packet->stream_index = stream->index;
        av_packet_rescale_ts(packet, codec_ctx->time_base, stream->time_base);

//        qDebug() << "B> Timestamp: " << pts << ". After rescaling: " << packet->dts * av_q2d(stream->time_base);

        // Write the compressed frame to the media file
        ret = av_interleaved_write_frame(fmt_ctx, packet);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */

        if (ret < 0) {
            qDebug() << "Error while writing output packet: " << ret;
            exit(1);
        }
    }

    return ret == AVERROR_EOF ? 1 : 0;
}

void FFmpeg::processVideoFile(QString input, QString output)
{
    int ret = -1;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    if ((ret = openInputFile(input.toLocal8Bit().data())) < 0){
        goto end;
    }

    if ((ret = openOutputFile(output.toLocal8Bit().data())) < 0){
        goto end;
    }

    // Read all packets
    while(av_read_frame(input_fmt_ctx, packet) >= 0){
        if(cancelled){
            break;
        }
        // Process audio stream
//        if(packet->stream_index == input_audio_stream_index){
//            ret = avcodec_send_packet(audio_decodec_ctx, packet);
//            if (ret < 0) {
//                av_log(NULL, AV_LOG_ERROR, "Decoding audio packet failed\n");
//                break;
//            }

//            av_packet_rescale_ts(packet,
//                                 input_fmt_ctx->streams[input_audio_stream_index]->time_base,
//                                 audio_decodec_ctx->time_base);

//            while (ret >= 0) {
//                ret = avcodec_receive_frame(audio_decodec_ctx, frame);
//                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
//                    break;
//                }
//                else if (ret < 0){
//                    goto end;
//                }

//                write_frame(output_fmt_ctx, audio_encodec_ctx,
//                            output_audio_stream, frame, packet);
//            }
//        }

        // Process video stream
        if(packet->stream_index == input_video_stream_index){
            // Get frame timestamp
            double pts = 0.0;
            if(packet->dts != AV_NOPTS_VALUE) {
                  pts = packet->dts;
            }
            pts *= av_q2d(input_fmt_ctx->streams[input_video_stream_index]->time_base);

            av_packet_rescale_ts(packet,
                                 input_fmt_ctx->streams[input_video_stream_index]->time_base,
                                 video_decodec_ctx->time_base);
//            qDebug() << "A> Timestamp: " << pts << ". After rescaling: " << packet->dts * av_q2d(video_decodec_ctx->time_base);

            ret = avcodec_send_packet(video_decodec_ctx, packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Decoding video packet failed\n");
                break;
            }

            while (ret >= 0) {
                if(cancelled){
                    break;
                }

                ret = avcodec_receive_frame(video_decodec_ctx, frame);

//                QString folder = QStandardPaths::writableLocation(
//                            QStandardPaths::StandardLocation::DocumentsLocation);
//                QImage img = OverlayGenerator::avFrameToQImage(frame, video_decodec_ctx);
//                img.save(folder + "/img.png");


                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
                    break;
                }
                else if (ret < 0){
                    goto end;
                }

                frame->pts = frame->best_effort_timestamp;

//                if(overlayGenerator){
//                    overlayGenerator->generateOverlayAt(frame, video_decodec_ctx, pts);
//                }

                writeFrame(output_fmt_ctx, video_encodec_ctx,
                            output_video_stream, frame, packet);

                emit progressChanged(currentFile, (int)((float)nb_video_frames_processed / (float)nb_video_frames * 100.0f));
                nb_video_frames_processed++;
            }
        }
    }

    if(!cancelled){
        av_write_trailer(output_fmt_ctx);
    }

end:

    av_packet_free(&packet);
    av_frame_free(&frame);

    avformat_close_input(&input_fmt_ctx);
    avformat_free_context(input_fmt_ctx);
    avcodec_close(audio_decodec_ctx);
    avcodec_close(video_decodec_ctx);
    av_free(output_audio_stream);

    avcodec_close(audio_encodec_ctx);
    avcodec_close(video_encodec_ctx);
    av_free(output_video_stream);

    qDebug() << "Number of counted video frames: " << nb_video_frames_processed;
}

QImage FFmpeg::getThumbnail(QString fileName){
    QImage thumbnail(0, 0, QImage::Format_RGBA8888);

    AVFormatContext* av_format_ctx = avformat_alloc_context();
    if(!av_format_ctx){
        qDebug() << "Could not allocate AVFormatContext for " + fileName;
        return thumbnail;
    }

    if(avformat_open_input(&av_format_ctx, fileName.toLocal8Bit().data(), NULL, NULL) != 0){
        qDebug() << "Could not open input file " + fileName;
        return thumbnail;
    }

    int video_stream_idx = -1;
    AVCodecParameters* av_codec_params;
    AVCodec* av_codec;
    AVCodecContext *av_codec_ctx;

    for(unsigned int i = 0; i < av_format_ctx->nb_streams; ++i){
        av_codec_params = av_format_ctx->streams[i]->codecpar;
        av_codec = avcodec_find_decoder(av_codec_params->codec_id);

        if(!av_codec){
            continue;
        }

        if(av_codec_params->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_idx = i;
            break;
        }
    }

    if(video_stream_idx == -1){
        qDebug() << "Could not find any video stream in " + fileName;
        return thumbnail;
    }
    else{
        av_codec_ctx = avcodec_alloc_context3(av_codec);
        if(!av_codec_ctx){
            qDebug() << "Could not allocate codec context for " + fileName;
            return thumbnail;
        }

        if(avcodec_parameters_to_context(av_codec_ctx, av_codec_params) < 0){
            qDebug() << "Could not initialize codec context for " + fileName;
            return thumbnail;
        }

        if(avcodec_open2(av_codec_ctx, av_codec, NULL) < 0){
            qDebug() << "Could not open codec for " + fileName;
            return thumbnail;
        }

        AVPacket* av_packet = av_packet_alloc();
        if(!av_packet){
            qDebug() << "Could not allocate packet for " + fileName;
            return thumbnail;
        }

        while(av_read_frame(av_format_ctx, av_packet) >= 0){
            if(av_packet->stream_index != video_stream_idx){
                continue;
            }

            if(avcodec_send_packet(av_codec_ctx, av_packet) < 0){
                qDebug() << "Failed to decodea packet for " + fileName;
                continue;
            }

            AVFrame* av_frame = av_frame_alloc();
            if(!av_frame){
                qDebug() << "Could not allocate frame for " + fileName;
                continue;
            }

            int response = avcodec_receive_frame(av_codec_ctx, av_frame);
            if(response == AVERROR(EAGAIN) || response == AVERROR_EOF){
                continue;
            }
            else if (response < 0){
                qDebug() << "Could not receive frame for " + fileName;
                continue;
            }

            // Scale the frame to fit thumbnail
            float scalingFactor = 120.0f / av_frame->height;

            thumbnail = OverlayGenerator::avFrameToQImage(av_frame, av_codec_ctx, scalingFactor);
            break;
        }
    }

    avformat_close_input(&av_format_ctx);
    avformat_free_context(av_format_ctx);
    avcodec_free_context(&av_codec_ctx);

    return thumbnail;
}

void FFmpeg::setOverlayX(int x)
{
    if(overlayGenerator){
        overlayGenerator->setOverlayX(x);
    }
}

void FFmpeg::setOverlayY(int y)
{
    if(overlayGenerator){
        overlayGenerator->setOverlayY(y);
    }
}

void FFmpeg::generateOverlay(QString folder, QString fileName, int overlayX, int overlayY)
{
    cancelled = false;
    bool success = false;
    QString outputFolder = folder;

    if(folder != "" && fileName != ""){
        outputFolder = outputFolder.replace("file://", "") + "/Edited";
        QDir outputDir(outputFolder);
        if(!outputDir.exists()){
            outputDir.mkpath(outputFolder);
        }

        nb_audio_frames = 0;
        nb_audio_frames_processed = 0;
        nb_video_frames = 0;
        nb_video_frames_processed = 0;
        currentFile = fileName;
        input_audio_stream_index = -1;
        input_video_stream_index = -1;

        QString input = folder + "/" + fileName;
        QString output = outputFolder + "/" + fileName;

        overlayGenerator->setOverlayX(overlayX);
        overlayGenerator->setOverlayY(overlayY);

        processVideoFile(input, output);

        if(cancelled){
            return;
        }

        success = true;
    }

    if(success){
        emit generationFinished(outputFolder);
    }
}

void FFmpeg::cancel(){
    cancelled = true;
}
