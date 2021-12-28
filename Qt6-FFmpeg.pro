QT += qml quick quickwidgets
QT += quickcontrols2
QT += multimedia multimediawidgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES += qml.qrc

SOURCES += \
    main.cpp

HEADERS +=

INCLUDEPATH += $$PWD/ffmpeg/include64

LIBS += -L$$PWD/ffmpeg/winlib64/ -lavcodec -lavfilter -lavformat -lswscale -lavutil -lswresample -lavdevice

# Additional import path used to resolve QML modules in Qt Creator's code model
QML2_IMPORT_PATH += C:\Qt\6.2.2\mingw_64\qml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
