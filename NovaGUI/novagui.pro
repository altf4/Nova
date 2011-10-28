TEMPLATE = app
TARGET = NovaGUI
QT += core \
    gui
HEADERS += novaconfig.h \
    novagui.h
SOURCES += novaconfig.cpp \
    main.cpp \
    novagui.cpp
FORMS += novaconfig.ui \
    novagui.ui
RESOURCES += 
INCLUDEPATH += ../NovaLibrary/src
LIBS += ../NovaLibrary/Debug/libNovaLibrary.a \
    /usr/lib/libboost_serialization.a \
    /usr/lib/libann.a \
    /usr/lib/liblog4cxx.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libaprutil-1.a
