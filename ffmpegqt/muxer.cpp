#include "muxer.h"

Muxer::Muxer()
{
}

void Muxer::log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

//    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
//           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
//           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
//           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
//           pkt->stream_index);
}

int Muxer::write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                       AVStream *st, AVFrame *frame, AVPacket *pkt)
{
    int ret;

    // send the frame to the encoder
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        qDebug() << "Error sending a frame to the encoder: " << ret;
        exit(1);
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(c, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            qDebug() << "Error encoding a frame: ", ret;
            exit(1);
        }

        /* rescale output packet timestamp values from codec to stream timebase */
        av_packet_rescale_ts(pkt, c->time_base, st->time_base);
        pkt->stream_index = st->index;

        /* Write the compressed frame to the media file. */
        log_packet(fmt_ctx, pkt);
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
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

void Muxer::add_stream(OutputStream *ost, AVFormatContext *oc,
                       const AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        qDebug() << "Could not find encoder for " << (int)codec_id;
        exit(1);
    }

    ost->tmp_pkt = av_packet_alloc();
    if (!ost->tmp_pkt) {
        qDebug() << "Could not allocate AVPacket";
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        qDebug() << "Could not alloc an encoding context";
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 352;
        c->height   = 288;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

AVFrame* Muxer::alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        qDebug() << "Error allocating an audio frame";
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            qDebug() << "Error allocating an audio buffer";
            exit(1);
        }
    }

    return frame;
}

void Muxer::open_audio(AVFormatContext *oc, const AVCodec *codec,
                       OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        qDebug() << "Could not open audio codec: " << ret;
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        qDebug() << "Could not copy the stream parameters";
        exit(1);
    }

    /* create resampler context */
    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        qDebug() << "Could not allocate resampler context\n";
        exit(1);
    }

    /* set options */
    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        qDebug() << "Failed to initialize the resampling context";
        exit(1);
    }
}

AVFrame* Muxer::get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) > 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

int Muxer::write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVFrame *frame;
    int ret;
    int dst_nb_samples;

    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            qDebug() << "Error while converting";
            exit(1);
        }
        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    return write_frame(oc, c, ost->st, frame, ost->tmp_pkt);
}

AVFrame* Muxer::alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

void Muxer::open_video(AVFormatContext *oc, const AVCodec *codec,
                       OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        qDebug() << "Could not open video codec: ", ret;
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        qDebug() << "Could not allocate video frame";
        exit(1);
    }

    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            qDebug() << "Could not allocate temporary picture";
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        qDebug() << "Could not copy the stream parameters";
        exit(1);
    }
}

void Muxer::fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

AVFrame* Muxer::get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) > 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                qDebug() << "Could not initialize the conversion context";
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

int Muxer::write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    return write_frame(oc, ost->enc, ost->st, get_video_frame(ost), ost->tmp_pkt);
}

void Muxer::close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    av_packet_free(&ost->tmp_pkt);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}

int Muxer::createVideo(QMap<QString, QString> args){
    OutputStream video_st = { 0 }, audio_st = { 0 };
    const AVOutputFormat *fmt;
    const char *filename;
    AVFormatContext *oc;
    const AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;

    if (args.count() < 2) {
        printf("usage: %s output_file\n"
               "API example program to output a media file with libavformat.\n"
               "This program generates a synthetic audio and video stream, encodes and\n"
               "muxes them into a file named output_file.\n"
               "The output format is automatically guessed according to the file extension.\n"
               "Raw images can also be output by using '%%d' in the filename.\n"
               "\n", (*args.begin()).toLocal8Bit().data());
        return 1;
    }

    filename = args["filename"].toLocal8Bit().data();
    args.remove("filename");

    QMap<QString, QString>::const_iterator it = args.constBegin();
    while (it != args.constEnd()) {
        av_dict_set(&opt, it.key().toLocal8Bit().data(), it.value().toLocal8Bit().data(), 0);
        ++it;
    }

    /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return 1;

    fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            qDebug() << "Could not open " << filename << " " << ret;
            return 1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        qDebug() << "Error occurred when opening output file: " << ret;
        return 1;
    }

    while (encode_video || encode_audio) {
        /* select the stream to encode */
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    if (have_video)
        close_stream(oc, &video_st);
    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        /* Close the output file. */
        avio_closep(&oc->pb);

    /* free the stream */
    avformat_free_context(oc);

    return 0;
}

AVFrame* Muxer::QImagetoAVFrame(QImage qImage){
    AVFrame *avFrame = av_frame_alloc();
    avFrame->width = qImage.width();
    avFrame->height = qImage.height();
    avFrame->format = AV_PIX_FMT_ARGB;
    avFrame->linesize[0] = qImage.width();

//    av_image_fill_arrays(avFrame->data, avFrame->linesize, (uint8_t*)qImage.bits(),
//                       AV_PIX_FMT_ARGB, avFrame->width, avFrame->height, 1);

    return avFrame;
}

QImage Muxer::AVFrametoQImage(AVFrame* avFrame){
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