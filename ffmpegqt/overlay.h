#ifndef OVERLAY_H
#define OVERLAY_H

#include <QQuickPaintedItem>
#include <QQuickItem>
#include <QPainter>
#include <QImage>
#include <QTimer>
#include <QPropertyAnimation>
#include <QRandomGenerator>
#include <QDateTime>

class Overlay : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(bool animationRunning READ getAnimationRunning WRITE setAnimationRunning NOTIFY animationRunningChanged)
    Q_PROPERTY(float sliderValue READ getSliderValue WRITE setSliderValue NOTIFY sliderValueChanged)

    const int SLIDER_ANIMATION_DURATION = 1000;

    int lastTimestampForNumeric = 0;
    int lastTimestampForShape = 0;
    bool isAnimationRunning = false;

    QImage frameImage;
    QImage overlayImage;
    QTimer numericValueTimer;
    QTimer shapeValueTimer;
    QPropertyAnimation* sliderAnimation;
    QRandomGenerator randomGenerator;
    QColor mainColor = "#f07502";

    int numericValue = 0;
    float shapeValue = 0.0f;
    float sliderValue = 0.0f;

    bool getAnimationRunning() { return isAnimationRunning; }
    void setAnimationRunning(bool running);

    float getSliderValue() { return sliderValue; }
    void setSliderValue(float value) { sliderValue = value; emit sliderValueChanged(); }

    void createSliderAnimation();
    void onNumericTimerTimeout();
    void onShapeTimerTimeout();
    void onSliderAnimation(const QVariant& value);

protected:
    void paint(QPainter *painter) override;

public:
    Overlay(QQuickItem *parent = nullptr);
    QImage getImageAtTimestamp(float timestamp);

signals:
    void sliderValueChanged();
    void animationRunningChanged();
};

#endif // OVERLAY_H
