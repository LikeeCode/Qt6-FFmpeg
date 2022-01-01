QT += qml quick
QT += quickcontrols2

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

RESOURCES += qml.qrc

SOURCES += \
    ffmpegqt/ffmpegqt.cpp \
    main.cpp

HEADERS += \
    ffmpegqt/ffmpegqt.h \
    ffmpegqt/outputstream.h

#INCLUDEPATH += $$PWD/ffmpeg/include64
INCLUDEPATH += /usr/include/x86_64-linux-gnu/

#Windows
#LIBS += -L$$PWD/ffmpeg/winlib64/ -lavcodec -lavfilter -lavformat -lswscale -lavutil -lswresample -lavdevice

#Linux
LIBS += -L/usr/lib/x86_64-linux-gnu/ -lavcodec -lavfilter -lavformat -lswscale -lavutil -lswresample -lavdevice
#LIBS += -lpthread -lm -lz -lrt -ldl

# Additional import path used to resolve QML modules in Qt Creator's code model
QML2_IMPORT_PATH +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
