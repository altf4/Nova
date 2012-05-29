TEMPLATE = app
TARGET = novagui
QT += core \
    gui
HEADERS += src/subnetPopup.h \
    src/loggerwindow.h \
    src/classifierPrompt.h \
    src/nova_manual.h \
    src/DialogPrompter.h \
    src/DialogPrompt.h \
    src/NovaComplexDialog.h \
    src/main.h \
    src/nodePopup.h \
    src/novaconfig.h \
    src/novagui.h \
    src/run_popup.h
SOURCES += src/subnetPopup.cpp \
    src/loggerwindow.cpp \
    src/classifierPrompt.cpp \
    src/nova_manual.cpp \
    src/DialogPrompter.cpp \
    src/DialogPrompt.cpp \
    src/NovaComplexDialog.cpp \
    src/main.cpp \
    src/nodePopup.cpp \
    src/novaconfig.cpp \
    src/novagui.cpp \
    src/run_popup.cpp
FORMS += UI/subnetPopup.ui \
    UI/loggerwindow.ui \
    UI/classifierPrompt.ui \
    UI/nova_manual.ui \
    UI/NovaComplexDialog.ui \
    UI/nodePopup.ui \
    UI/novaconfig.ui \
    UI/novagui.ui \
    UI/run_popup.ui
RESOURCES += 
INCLUDEPATH += ../NovaLibrary/src
INCLUDEPATH += ../Nova_UI_Core/src
QMAKE_CXXFLAGS += -std=c++0x
CONFIG(debug, debug|release):LIBS += ../NovaLibrary/Debug/libNovaLibrary.a
else:LIBS += ../NovaLibrary/Release/libNovaLibrary.a
CONFIG(debug, debug|release):LIBS += ../Nova_UI_Core/Debug/libNova_UI_Core.so
else:LIBS += ../Nova_UI_Core/Release/libNova_UI_Core.so
LIBS += /usr/lib/libboost_serialization.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libann.a \
    /usr/lib/libaprutil-1.a \
    /usr/lib/libpcap.a
UI_DIR = UI_headers/
QMAKE_CLEAN += $(TARGET)
CONFIG += no_keywords
CONFIG += link_pkgconfig
PKGCONFIG += libnotify
