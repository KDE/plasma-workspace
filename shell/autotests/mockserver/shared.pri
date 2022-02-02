QT += testlib waylandclient-private
CONFIG += testcase wayland-scanner
QMAKE_USE += wayland-server

WAYLANDSERVERSOURCES += \
    $$PWD/../../../../src/3rdparty/protocol/wayland.xml \
    $$PWD/../../../../src/3rdparty/protocol/xdg-output-unstable-v1.xml \
    $$PWD/../../../../src/3rdparty/protocol/xdg-shell.xml \
    $$PWD/../../../../src/3rdparty/protocol/text-input-unstable-v2.xml

INCLUDEPATH += ../shared

HEADERS += \
    $$PWD/corecompositor.h \
    $$PWD/coreprotocol.h \
    $$PWD/datadevice.h \
    $$PWD/mockcompositor.h \
    $$PWD/xdgoutputv1.h \
    $$PWD/xdgshell.h \
    $$PWD/textinput.h

SOURCES += \
    $$PWD/corecompositor.cpp \
    $$PWD/coreprotocol.cpp \
    $$PWD/datadevice.cpp \
    $$PWD/mockcompositor.cpp \
    $$PWD/xdgoutputv1.cpp \
    $$PWD/xdgshell.cpp \
    $$PWD/textinput.cpp
