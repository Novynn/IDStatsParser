#-------------------------------------------------
#
# Project created by QtCreator 2015-02-15T20:36:34
#
#-------------------------------------------------

QT       += core network

CONFIG += console

TARGET = IDStatsParser

INCLUDEPATH += zlib/include

LIBS += -L"$$_PRO_FILE_PWD_/zlib/" -L"$$_PRO_FILE_PWD_/zlib/libs/" -lzlib1

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++14

SOURCES += main.cpp \
    entconnection.cpp \
    w3replay.cpp \
    shared/qbytearraybuilder.cpp

HEADERS += \
    entconnection.h \
    entgame.h \
    w3replay.h \
    entplayer.h \
    shared/qbytearraybuilder.h \
    w3idplayer.h \
    w3idreplay.h \
    shared/boolinq.h \
    shared/functions.h

FORMS +=

RESOURCES +=
