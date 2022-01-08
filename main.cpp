#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QStandardPaths>

#include "ffmpegqt/muxer.h"
#include "ffmpegqt/remuxer.h"
#include "ffmpegqt/transcoder.h"
#include "ffmpegqt/videomaster.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    // Overlay
    OverlayGenerator overlayGenerator(&engine);
    engine.rootContext()->setContextProperty("OVERLAY_NUMERIC", overlayGenerator.numericValue);
    engine.rootContext()->setContextProperty("OVERLAY_SHAPE", overlayGenerator.shapeValue);
    engine.rootContext()->setContextProperty("OVERLAY_SLIDER", overlayGenerator.sliderValue);

    // FFmpeg
    VideoMaster videoMaster;
    videoMaster.setOverlayGenerator(&overlayGenerator);
    qmlRegisterType<VideoMaster>("VideoMaster", 1, 0, "VideoMaster");
    engine.rootContext()->setContextProperty("videoMaster", &videoMaster);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    // Muxer
//    QString fileName = QStandardPaths::writableLocation(
//                QStandardPaths::StandardLocation::DocumentsLocation) +
//                "/muxer.mp4";
//    QMap<QString, QString> args{{"filename", fileName},
//                                {"-c:v", "libx264"}};
//    Muxer muxer;
//    muxer.mux(args);

    // Transcoder
    QString input = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017.MP4";
//                "/VID_20220105_125833.mp4";

    QString output = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017_EDITED.MP4";
//                "/VID_20220105_125833_EDITED.mp4";

    Remuxer remuxer;
    remuxer.remux(input, output);

//    Transcoder transcoder;
//    transcoder.transcode(input, output);

    // Video master
//    videoMaster.generateOverlay(input, output);

    return app.exec();
}
