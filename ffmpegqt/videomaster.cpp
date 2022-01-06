#include "videomaster.h"

VideoMaster::VideoMaster()
{

}

int VideoMaster::generateOverlayVideo(QString input, QString output)
{
    int ret = -1;
    AVPacket *packet = NULL;
    unsigned int i;

    if ((ret = open_input_file(input.toLocal8Bit().data())) < 0){
        goto end;
    }

    if ((ret = open_output_file(output.toLocal8Bit().data())) < 0){
        goto end;
    }

//    if (!(packet = av_packet_alloc())){
//        goto end;
//    }

//    // Read all packets
//    while(1){
//        if ((ret = av_read_frame(input_fmt_ctx, packet)) < 0){
//            break;
//        }

//        // Process audio stream
//        if(packet->stream_index == input_audio_stream_index){
//            av_packet_rescale_ts(packet,
//                                 input_fmt_ctx->streams[input_audio_stream_index]->time_base,
//                                 audio_codec_ctx->time_base);

//            ret = avcodec_send_packet(audio_codec_ctx, packet);
//            if (ret < 0) {
//                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
//                break;
//            }

//            while (ret >= 0) {
//                ret = avcodec_receive_frame(audio_codec_ctx, audio_frame);
//                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
//                    break;
//                }
//                else if (ret < 0){
//                    goto end;
//                }

//                audio_frame->pts = audio_frame->best_effort_timestamp;
//                ret = filter_encode_write_frame(audio_frame, stream_index);
//            }
//        }
//    }

end:

    avformat_close_input(&input_fmt_ctx);
    avformat_free_context(input_fmt_ctx);

    return ret;
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

            audio_frame = av_frame_alloc();
            if (!audio_frame){
                return AVERROR(ENOMEM);
            }
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

            video_frame = av_frame_alloc();
            if (!video_frame){
                return AVERROR(ENOMEM);
            }
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
