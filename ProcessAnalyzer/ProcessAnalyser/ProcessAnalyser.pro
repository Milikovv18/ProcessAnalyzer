QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

LIBS += Imagehlp.lib
LIBS += Mswsock.lib
LIBS += Ws2_32.lib

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    animestackedwidget.cpp \
    detective.cpp \
    helper.cpp \
    hookermanager.cpp \
    injector.cpp \
    main.cpp \
    mainwindow.cpp \
    metacompute.cpp \
    moduleobject.cpp \
    network.cpp

HEADERS += \
    animestackedwidget.h \
    detective.h \
    helper.h \
    hookermanager.h \
    injector.h \
    mainwindow.h \
    metacompute.h \
    moduleobject.h \
    network.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Icons.qrc
