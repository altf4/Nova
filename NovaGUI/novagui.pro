TEMPLATE = app
TARGET = NovaGUI
QT += core \
    gui
HEADERS += src/main.h \
    src/nodePopup.h \
    src/novaconfig.h \
    src/novagui.h \
    src/portPopup.h \
    src/run_popup.h
SOURCES += src/main.cpp \
    src/nodePopup.cpp \
    src/novaconfig.cpp \
    src/novagui.cpp \
    src/portPopup.cpp \
    src/run_popup.cpp
FORMS += UI/nodePopup.ui \
    UI/novaconfig.ui \
    UI/novagui.ui \
    UI/portPopup.ui \
    UI/run_popup.ui
RESOURCES += 
INCLUDEPATH += ../NovaLibrary/src
LIBS += ../NovaLibrary/Debug/libNovaLibrary.a \
    /usr/lib/libboost_serialization.a \
    /usr/lib/libann.a \
    /usr/lib/liblog4cxx.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libaprutil-1.a
UI_DIR = UI_headers/ 
