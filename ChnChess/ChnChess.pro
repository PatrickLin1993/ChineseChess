#-------------------------------------------------
#
# Project created by QtCreator 2017-10-16T16:20:26
#
#-------------------------------------------------

QT       += core gui
QT       += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ChnChess
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    chessboard.cpp \
    computerplayer.cpp \
    maindialog.cpp \
    saveddialog.cpp \
    startingdialog.cpp \
    main.cpp

HEADERS  += \
    chessboard.h \
    chinesechess.h \
    computerplayer.h \
    maindialog.h \
    saveddialog.h \
    startingdialog.h

FORMS    += \
    maindialog.ui \
    saveddialog.ui \
    startingdialog.ui

RESOURCES += \
    resource.qrc

RC_FILE += myicon.rc
