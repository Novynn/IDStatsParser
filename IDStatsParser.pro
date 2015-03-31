#-------------------------------------------------
#
# Project created by QtCreator 2015-02-15T20:36:34
#
#-------------------------------------------------

QT       += core sql network
QT += xmlpatterns
QT       -= gui

TARGET = IDStatsParser
CONFIG   += console
CONFIG   -= app_bundle

INCLUDEPATH += zlib/include

LIBS += -L"$$_PRO_FILE_PWD_/zlib/" -L"$$_PRO_FILE_PWD_/zlib/libs/" -lzlib1

QTPLUGIN += QSQLMYSQL

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++14

SOURCES += main.cpp \
    entconnection.cpp \
    w3replay.cpp \
    statsdatabase.cpp \
    application.cpp \
    shared/qbytearraybuilder.cpp

HEADERS += \
    entconnection.h \
    entgame.h \
    w3replay.h \
    entplayer.h \
    statsdatabase.h \
    application.h \
    shared/qbytearraybuilder.h \
    w3idplayer.h \
    w3idreplay.h \
    shared/boolinq.h
