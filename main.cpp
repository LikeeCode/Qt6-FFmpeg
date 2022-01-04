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

    QString fileName = QStandardPaths::writableLocation(
                QStandardPaths::StandardLocation::DocumentsLocation) +
                "/DJI_0017.MP4";

    int width, height;
    unsigned char *data;
//    muxer.load_frame(fileName.toLocal8Bit().data(), &width, &height, &data);
    qDebug() << muxer.getSliderValueAt(0.0);
    qDebug() << muxer.getSliderValueAt(0.25);
    qDebug() << muxer.getSliderValueAt(0.5);
    qDebug() << muxer.getSliderValueAt(0.75);
    qDebug() << muxer.getSliderValueAt(1.0);
    qDebug() << muxer.getSliderValueAt(1.25);
    qDebug() << muxer.getSliderValueAt(1.5);
    qDebug() << muxer.getSliderValueAt(1.75);
    qDebug() << muxer.getSliderValueAt(2.0);

    muxer.renderQml(&engine);

    return app.exec();
}
