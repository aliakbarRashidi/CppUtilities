CONFIG -= qt

TEMPLATE = lib
DEFINES += CPPUTILITIES_LIBRARY
VERSION = 0.1.0

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    cpputilities.cpp \
    debuging.cpp \
    signals_slots.cpp \
    threading.cpp

HEADERS += \
    cpputilities.h \
    cpputilities_global.h \
    debuging.h \
    signals_slots.h \
    threading.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
