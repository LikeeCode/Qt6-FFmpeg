#ifndef FFMPEG_H
#define FFMPEG_H

// Compatibility with C and C99 standards
#ifndef INT64_C
#define INT64_C
#define UINT64_C
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QStandardPaths>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QDir>
#include <QDebug>

#include "overlaygenerator.h"

class FFmpeg : public QObject
{
    Q_OBJECT

private:
    bool cancelled = false;

    AVFormatContext *input_fmt_ctx = nullptr;
    AVFormatContext *output_fmt_ctx = nullptr;

    AVCodecContext *audio_decodec_ctx = nullptr;
    AVCodecContext *video_decodec_ctx = nullptr;

    AVCodecContext *audio_encodec_ctx = nullptr;
    AVCodecContext *video_encodec_ctx = nullptr;

    AVStream *output_audio_stream = nullptr;
    AVStream *output_video_stream = nullptr;

    int input_audio_stream_index = -1;
    int input_video_stream_index = -1;

    int nb_audio_frames = 0;
    int nb_audio_frames_processed = 0;
    int nb_video_frames = 0;
    int nb_video_frames_processed = 0;
    QString currentFile = "";

    OverlayGenerator* overlayGenerator = nullptr;

public:
    FFmpeg();

    void setOverlayGenerator(OverlayGenerator* generator);

private:
    int openInputFile(const char *filename);
    int openOutputFile(const char *filename);
    int writeFrame(AVFormatContext *fmt_ctx, AVCodecContext *codec_ctx,
                    AVStream *stream, AVFrame* frame, AVPacket *packet);
    void processVideoFile(QString input, QString output);
    QImage getThumbnail(QString fileName);

public slots:
    void setOverlayX(int x);
    void setOverlayY(int y);
    void generateOverlay(QString folder, QString fileName,
                         int overlayX = 0, int overlayY = 0);
    void cancel();

signals:
    void thumbnailReady(QString fileName, QImage thumbnail);
    void progressChanged(QString fileName, int progress);
    void generationFinished(QString resultFolder);
};

#endif // FFMPEG_H
