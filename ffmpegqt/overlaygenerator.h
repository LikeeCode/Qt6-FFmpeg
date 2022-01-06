#ifndef OVERLAYGENERATOR_H
#define OVERLAYGENERATOR_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QStandardPaths>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlProperty>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QPainter>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QDebug>

#define SLIDER_ANIM_DUR 1000 // milliseconds

class OverlayGenerator
{
private:
    QQmlApplicationEngine *engine;
    QQuickView* view;
    QRandomGenerator randomGenerator;
    QPropertyAnimation sliderAnimation;

    QString getNumericValueAt(float timestamp);
    float getShapeValueAt(float timestamp);
    float getSliderValueAt(float timestamp);
    void createSliderAnimation();

public:
    OverlayGenerator(QQmlApplicationEngine *e);

    QImage generateOverlayAt(double timestamp = 0.0);
};

#endif // OVERLAYGENERATOR_H
