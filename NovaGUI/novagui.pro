TEMPLATE = app
TARGET = NovaGUI
QT += core \
    gui
HEADERS += nodePopup.h \
    portPopup.h \
    run_popup.h \
    novaconfig.h \
    novagui.h
SOURCES += nodePopup.cpp \
    portPopup.cpp \
    run_popup.cpp \
    novaconfig.cpp \
    main.cpp \
    novagui.cpp
FORMS += nodePopup.ui \
    portPopup.ui \
    run_popup.ui \
    novaconfig.ui \
    novagui.ui
RESOURCES += 
INCLUDEPATH += ../NovaLibrary/src
LIBS += ../NovaLibrary/Debug/libNovaLibrary.a \
    /usr/lib/libboost_serialization.a \
    /usr/lib/libann.a \
    /usr/lib/liblog4cxx.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libaprutil-1.a
