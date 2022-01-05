#include "videomaster.h"

VideoMaster::VideoMaster()
{

}

int VideoMaster::openInputFile(QString filename)
{
    int ret = -1;

    input_fmt_ctx = NULL;
    if ((ret = avformat_open_input(&input_fmt_ctx, filename.toLocal8Bit().data(), NULL, NULL)) < 0) {
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

        if(codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO){
            codec_ctx->framerate = av_guess_frame_rate(input_fmt_ctx, stream, NULL);

            // Open decoder
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }

            audio_decoding_ctx = codec_ctx;
            input_audio_stream_index = i;
        }
        else if(codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO){
            codec_ctx->framerate = av_guess_frame_rate(input_fmt_ctx, stream, NULL);

            // Open decoder
            ret = avcodec_open2(codec_ctx, dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }

            video_decoding_ctx = codec_ctx;
            input_video_stream_index = i;
        }
    }

    av_dump_format(input_fmt_ctx, 0, filename.toLocal8Bit().data(), 0);

    return 0;
}

int VideoMaster::openOutputFile(QString filename)
{
    int ret = -1;
    const AVCodec *audio_encoder, *video_encoder;

    output_fmt_ctx = NULL;
    avformat_alloc_output_context2(&output_fmt_ctx, NULL, NULL, filename.toLocal8Bit().data());
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

        audio_encoder = avcodec_find_encoder(audio_decoding_ctx->codec_id);

        if (!audio_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary audio encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        audio_encoding_ctx = avcodec_alloc_context3(audio_encoder);
        if (!audio_encoding_ctx) {
            av_log(NULL, AV_LOG_FATAL, "Failed to allocate the audio encoder context\n");
            return AVERROR(ENOMEM);
        }

        audio_encoding_ctx->sample_rate = audio_decoding_ctx->sample_rate;
        audio_encoding_ctx->channel_layout = audio_decoding_ctx->channel_layout;
        audio_encoding_ctx->channels = av_get_channel_layout_nb_channels(audio_encoding_ctx->channel_layout);

        // Take first format from list of supported formats
        audio_encoding_ctx->sample_fmt = audio_encoder->sample_fmts[0];
        audio_encoding_ctx->time_base = (AVRational){1, audio_encoding_ctx->sample_rate};

        if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
            audio_encoding_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret = avcodec_open2(audio_encoding_ctx, audio_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_audio_stream->codecpar, audio_encoding_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output audio stream");
            return ret;
        }

        output_audio_stream->time_base = audio_encoding_ctx->time_base;
    }

    if(input_video_stream_index != -1){
        output_video_stream = avformat_new_stream(output_fmt_ctx, NULL);
        if (!output_video_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output video stream\n");
            return AVERROR_UNKNOWN;
        }

        video_encoder = avcodec_find_encoder(video_decoding_ctx->codec_id);

        if (!video_encoder) {
            av_log(NULL, AV_LOG_FATAL, "Necessary video encoder not found\n");
            return AVERROR_INVALIDDATA;
        }

        video_encoding_ctx = avcodec_alloc_context3(video_encoder);
        if (!video_encoding_ctx) {
            av_log(NULL, AV_LOG_FATAL, "Failed to allocate the video encoder context\n");
            return AVERROR(ENOMEM);
        }

        video_encoding_ctx->height = video_decoding_ctx->height;
        video_encoding_ctx->width = video_decoding_ctx->width;
        video_encoding_ctx->sample_aspect_ratio = video_decoding_ctx->sample_aspect_ratio;

        // Take first format from list of supported formats
        if (video_encoder->pix_fmts)
            video_encoding_ctx->pix_fmt = video_encoder->pix_fmts[0];
        else
            video_encoding_ctx->pix_fmt = video_decoding_ctx->pix_fmt;

        // Video time_base can be set to whatever is handy and supported by encoder
        video_encoding_ctx->time_base = av_inv_q(video_decoding_ctx->framerate);

        if (output_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
            video_encoding_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret = avcodec_open2(video_encoding_ctx, video_encoder, NULL);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder");
            return ret;
        }

        ret = avcodec_parameters_from_context(output_video_stream->codecpar, video_encoding_ctx);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output video stream");
            return ret;
        }

        output_video_stream->time_base = video_encoding_ctx->time_base;
    }

    av_dump_format(output_fmt_ctx, 0, filename.toLocal8Bit().data(), 1);

    if (!(output_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_fmt_ctx->pb, filename.toLocal8Bit().data(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename.toLocal8Bit().data());
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
