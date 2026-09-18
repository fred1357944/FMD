QT += core gui quick testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_fmd

INCLUDEPATH += ../src
SOURCES += \
    tst_fmd.cpp \
    ../src/backend.cpp \
    ../src/codeblocks.cpp \
    ../src/frontmatter.cpp \
    ../src/markdownhighlighter.cpp \
    ../src/mindmap.cpp \
    ../src/pluginhost.cpp \
    ../src/uilocale.cpp
HEADERS += \
    ../src/backend.h \
    ../src/codeblocks.h \
    ../src/frontmatter.h \
    ../src/markdownhighlighter.h \
    ../src/mindmap.h \
    ../src/pluginhost.h \
    ../src/uilocale.h

QT += widgets printsupport quickcontrols2 quickdialogs2 dbus
