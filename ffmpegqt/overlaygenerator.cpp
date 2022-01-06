#include "overlaygenerator.h"

OverlayGenerator::OverlayGenerator(QQmlApplicationEngine *e) : engine(e)
{
    createSliderAnimation();
}

QString OverlayGenerator::getNumericValueAt(float timestamp)
{
    static int lastTimestamp = 0;
    static int lastValue = 0;

    int currentTimestamp = static_cast<int>(timestamp / 1000);
    if(currentTimestamp != lastTimestamp){
        lastTimestamp = currentTimestamp;
        lastValue = randomGenerator.bounded(99, 999);
    }

    return QString::number(lastValue);
}

float OverlayGenerator::getShapeValueAt(float timestamp)
{
    static int lastTimestamp = 0;
    static float lastValue = 0.0f;

    int currentTimestamp = static_cast<int>(timestamp / 300);
    if(currentTimestamp != lastTimestamp){
        lastTimestamp = currentTimestamp;
        lastValue = randomGenerator.bounded(1.0);
    }

    return lastValue;
}

float OverlayGenerator::getSliderValueAt(float timestamp)
{
    int timestampInMs = timestamp * 1000;
    int currentTime = timestampInMs % SLIDER_ANIM_DUR;
    sliderAnimation.setCurrentTime(currentTime);
    return sliderAnimation.currentValue().toFloat();
}

void OverlayGenerator::createSliderAnimation()
{
    sliderAnimation.setDuration(SLIDER_ANIM_DUR);
    sliderAnimation.setKeyValueAt(0.0, 0.0);
    sliderAnimation.setKeyValueAt(0.5, 1.0);
    sliderAnimation.setKeyValueAt(1.0, 0.0);
}

QImage OverlayGenerator::generateOverlayAt(double timestamp)
{
    auto numericValue = getNumericValueAt(timestamp);
    auto shapeValue = getShapeValueAt(timestamp);
    auto sliderValue = getSliderValueAt(timestamp);

    view = new QQuickView(engine, nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/qml/Overlay.qml")));
    view->setColor(QColorConstants::Transparent);
    view->rootContext()->setContextProperty("OVERLAY_NUMERIC", numericValue);
    view->rootContext()->setContextProperty("OVERLAY_SHAPE", shapeValue);
    view->rootContext()->setContextProperty("OVERLAY_SLIDER", sliderValue);

    QImage overlayImage = view->grabWindow();

    return overlayImage;
}
