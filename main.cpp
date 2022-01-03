#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QStandardPaths>

#include "ffmpegqt/ffmpegqt.h"
#include "ffmpegqt/muxer.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
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

    QString fileName = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017.MP4";

    int width, height;
    unsigned char* data;
    muxer.load_frame(fileName.toLocal8Bit().data(), &width, &height, &data);
//    muxer.getFrame(fileName, "");

    return app.exec();
}
