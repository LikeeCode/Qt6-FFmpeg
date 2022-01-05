#include "videomaster.h"

VideoMaster::VideoMaster()
{

}

int VideoMaster::openInputFile(QString filename)
{
    int ret;

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
