TEMPLATE = app
TARGET = NovaGUI
QT += core \
    gui

# PRE_TARGETDEPS += /usr/lib/libann.a
HEADERS += src/classifierPrompt.h \
    src/nova_manual.h \
    src/DialogPrompter.h \
    src/DialogPrompt.h \
    src/NovaComplexDialog.h \
    src/main.h \
    src/nodePopup.h \
    src/novaconfig.h \
    src/novagui.h \
    src/run_popup.h
SOURCES += src/classifierPrompt.cpp \
    src/nova_manual.cpp \
    src/DialogPrompter.cpp \
    src/DialogPrompt.cpp \
    src/NovaComplexDialog.cpp \
    src/main.cpp \
    src/nodePopup.cpp \
    src/novaconfig.cpp \
    src/novagui.cpp \
    src/run_popup.cpp
FORMS += UI/classifierPrompt.ui \
    UI/nova_manual.ui \
    UI/NovaComplexDialog.ui \
    UI/nodePopup.ui \
    UI/novaconfig.ui \
    UI/novagui.ui \
    UI/run_popup.ui
RESOURCES += 
INCLUDEPATH += ../NovaLibrary/src
CONFIG(debug, debug|release):LIBS += ../NovaLibrary/Debug/libNovaLibrary.a
else:LIBS += ../NovaLibrary/Release/libNovaLibrary.a
LIBS += /usr/lib/libboost_serialization.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libann.a \
    /usr/lib/libaprutil-1.a
UI_DIR = UI_headers/
QMAKE_CLEAN += $(TARGET)
CONFIG += no_keywords
CONFIG += link_pkgconfig
PKGCONFIG += libnotify
