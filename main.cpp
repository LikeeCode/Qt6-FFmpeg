#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QStandardPaths>

#include "ffmpegqt/ffmpegqt.h"
#include "ffmpegqt/muxer.h"
#include "ffmpegqt/transcoder.h"
#include "ffmpegqt/videomaster.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("OVERLAY_NUMERIC", "");
    engine.rootContext()->setContextProperty("OVERLAY_SHAPE", 0.0);
    engine.rootContext()->setContextProperty("OVERLAY_SLIDER", 0.0);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

//    QString fileName = QStandardPaths::writableLocation(
//                QStandardPaths::StandardLocation::DocumentsLocation) +
//                "/muxing.mp4";

//    FFmpegQt ffmpegQt;
//    ffmpegQt.createVideo(fileName, "libx264");

//    QMap<QString, QString> args{{"filename", fileName},
//                                {"-c:v", "libx264"}};
    Muxer muxer;
//    muxer.createVideo(args);

    QString input = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017.MP4";
//                "/VID_20220105_125833.mp4";

    int width, height;
    unsigned char *data;
//    muxer.load_frame(input.toLocal8Bit().data(), &width, &height, &data);
//    muxer.renderQml(&engine);

    QString output = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017_TRANSCODED.MP4";
//                "/VID_20220105_125833_TRANSCODED.mp4";

//    Transcoder transcoder(&engine);
//    transcoder.transcode(input, output);

    VideoMaster videoMaster(&engine);
    videoMaster.generateOverlayVideo(input, output);

    return app.exec();
}
