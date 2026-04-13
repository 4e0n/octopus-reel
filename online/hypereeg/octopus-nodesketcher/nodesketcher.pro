QT += core gui widgets

CONFIG += c++17
TEMPLATE = app
TARGET = nodesketcher

SOURCES += \
    main.cpp

HEADERS += \
    nodetypes.h \
    patchgraph.h \
    patchregistry.h \
    patchvalidator.h \
    patchmainwindow.h
