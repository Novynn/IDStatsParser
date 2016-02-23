#-------------------------------------------------
#
# Project created by QtCreator 2015-05-17T14:44:36
#
#-------------------------------------------------

QT       += core sql network
QT       += xmlpatterns
QT       -= gui

TARGET = IDReplayScraper
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += zlib/include

LIBS += -L"$$_PRO_FILE_PWD_/zlib/" -L"$$_PRO_FILE_PWD_/zlib/libs/" -lzlib1

QTPLUGIN += QSQLMYSQL

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++14

SOURCES += \
    entconnection.cpp \
    w3replay.cpp \
    statsdatabase.cpp \
    shared/qbytearraybuilder.cpp \
    idreplayscraper.cpp \
    scraperapplication.cpp

HEADERS += \
    entconnection.h \
    entgame.h \
    w3replay.h \
    entplayer.h \
    statsdatabase.h \
    shared/qbytearraybuilder.h \
    w3idplayer.h \
    w3idreplay.h \
    shared/boolinq.h \
    shared/functions.h \
    scraperapplication.h
