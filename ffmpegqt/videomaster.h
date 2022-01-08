#ifndef VIDEOMASTER_H
#define VIDEOMASTER_H

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
#include <QImage>
#include <QStandardPaths>
#include <QPainter>
#include <QQmlApplicationEngine>
#include <QDebug>

#include "overlaygenerator.h"

class VideoMaster : public QObject
{
    Q_OBJECT

private:
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

    int open_input_file(const char *filename);
    int open_output_file(const char *filename);

    int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *codec_ctx,
                    AVStream *stream, AVFrame* frame, AVPacket *packet);

    OverlayGenerator* overlayGenerator = nullptr;

public:
    VideoMaster();

    void setOverlayGenerator(OverlayGenerator* generator);
    int generateOverlay(QString input, QString output);

    static QImage avFrameToQImage(AVFrame* frame);
    static void QImageToAVFrame(QImage image, AVFrame* frame);

signals:
    void progressChanged(int progress);
    void generationFinished(QString resultFolder);

public slots:
    void getThumbnails(QString folder);
    void setOverlayX(int x);
    void setOverlayY(int y);
    void generateOverlay(QString folder, QStringList files);
    void cancel();
};

#endif // VIDEOMASTER_H
