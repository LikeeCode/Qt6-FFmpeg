#include "overlay.h"

Overlay::Overlay(QQuickItem *parent): QQuickPaintedItem(parent)
{
    frameImage = QImage(":/Qml/Img/overlay_frame.png");
    overlayImage = QImage(":/Qml/Img/overlay_frame.png");

    connect(&numericValueTimer, &QTimer::timeout, this, &Overlay::onNumericTimerTimeout);
    connect(&shapeValueTimer, &QTimer::timeout, this, &Overlay::onShapeTimerTimeout);

    numericValueTimer.setInterval(1000);
    numericValueTimer.setSingleShot(false);

    shapeValueTimer.setInterval(300);
    shapeValueTimer.setSingleShot(false);

    createSliderAnimation();
}

QImage Overlay::getImageAtTimestamp(float timestamp)
{
    // Numeric value
    int currentTimestamp = timestamp * 1000 / 1000;
    if(currentTimestamp != lastTimestampForNumeric){
        lastTimestampForNumeric = currentTimestamp;
        numericValue = randomGenerator.bounded(99, 999);
    }

    // Shape value
    currentTimestamp = timestamp * 1000 / 300;
    if(currentTimestamp != lastTimestampForShape){
        lastTimestampForShape = currentTimestamp;
        shapeValue = (float)randomGenerator.generateDouble();
    }

    // Slider value
    sliderAnimation->setCurrentTime(timestamp * 1000);
    sliderValue = sliderAnimation->currentValue().toFloat();

    QPainter tmp;
    paint(&tmp);

    return overlayImage;
}

void Overlay::createSliderAnimation()
{
    sliderAnimation = new QPropertyAnimation(this, "sliderValue");
    sliderAnimation->setDuration(SLIDER_ANIMATION_DURATION);
    sliderAnimation->setLoopCount(-1);
    sliderAnimation->setKeyValueAt(0.0, 0.0);
    sliderAnimation->setKeyValueAt(0.5, 1.0);
    sliderAnimation->setKeyValueAt(1.0, 0.0);
    connect(sliderAnimation, &QPropertyAnimation::valueChanged, this, &Overlay::onSliderAnimation);
}

void Overlay::onNumericTimerTimeout()
{
    numericValue = randomGenerator.bounded(99, 999);
    update();
}

void Overlay::onShapeTimerTimeout()
{
    shapeValue = (float)randomGenerator.generateDouble();
    update();
}

void Overlay::onSliderAnimation(const QVariant& value)
{
    sliderValue = value.toFloat();
    update();
}

void Overlay::paint(QPainter *painter)
{
    // Create another painter to be able to save image
    overlayImage = frameImage.copy();
    QPainter overlayImagePainter(&overlayImage);

    // Numeric value
    QFont font = overlayImagePainter.font();
    font.setPixelSize(100);
    overlayImagePainter.setFont(font);
    overlayImagePainter.setPen(mainColor);
    overlayImagePainter.drawText(80, 160, QString::number(numericValue));

    // Gradient
    QLinearGradient m_gradient(QPoint(300, 80), QPoint(300, 160));
    m_gradient.setColorAt(shapeValue, Qt::transparent);
    m_gradient.setColorAt(1.0, mainColor);
    overlayImagePainter.fillRect(QRect(380, 86, 140, 76), m_gradient);

    // Slider
    overlayImagePainter.drawRect(80, 200, 440, 16);
    overlayImagePainter.fillRect((80 + 424 * sliderValue), 200, 16, 16, mainColor);

    QRectF bounding_rect = boundingRect();
    QImage scaled = overlayImage.scaledToHeight(bounding_rect.height());
    QPointF center = bounding_rect.center() - scaled.rect().center();

    if(center.x() < 0)
        center.setX(0);
    if(center.y() < 0)
        center.setY(0);

    painter->drawImage(center, overlayImage);
}

void Overlay::setAnimationRunning(bool running){
    isAnimationRunning = running;

    if(isAnimationRunning){
        numericValueTimer.start();
        shapeValueTimer.start();
        sliderAnimation->start();
    }
    else{
        numericValueTimer.stop();
        shapeValueTimer.stop();
        sliderAnimation->stop();
    }

    emit animationRunningChanged();
}
