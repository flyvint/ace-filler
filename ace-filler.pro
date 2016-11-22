QT += core xml
QT -= gui

CONFIG += c++11

TARGET = ace-filler
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    aceofferfiller.cpp

HEADERS += \
    aceofferfiller.h
