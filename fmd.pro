QT += core gui widgets printsupport qml quick quickcontrols2 quickdialogs2 dbus

CONFIG += c++17 release
TARGET = fmd
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/codeblocks.h \
    src/frontmatter.h \
    src/markdownhighlighter.h \
    src/mindmap.h \
    src/pluginhost.h \
    src/systemtheme.h \
    src/uilocale.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/codeblocks.cpp \
    src/frontmatter.cpp \
    src/markdownhighlighter.cpp \
    src/mindmap.cpp \
    src/pluginhost.cpp \
    src/systemtheme.cpp \
    src/uilocale.cpp

RESOURCES += src/resources.qrc

macx {
    ICON = icons/fmd.icns
}
