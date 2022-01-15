#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QStandardPaths>

#include "ffmpegqt/ffmpeg.h"
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

    QString folder = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation);
    QString inputFile = "DJI_0017.MP4";
    QString outputFile = "DJI_0017_EDITED.MP4";

    // Remuxer
    Remuxer remuxer;
    remuxer.remux(folder + "/" + inputFile,
                  folder + "/" + outputFile);

    // FFmpeg and Overlay generator
//    OverlayGenerator overlayGenerator;
//    FFmpeg ffMpeg;
//    ffMpeg.setOverlayGenerator(&overlayGenerator);
//    ffMpeg.setOverlayX(100);
//    ffMpeg.setOverlayY(100);
//    ffMpeg.generateOverlay(folder, inputFile);

    const QUrl url(QStringLiteral("qrc:/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
