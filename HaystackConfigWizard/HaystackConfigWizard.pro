TEMPLATE = app
TARGET = haystackconfigwizard
QT += core \
    gui
HEADERS += src/forms/ipRangeSelectionForm.h \
    src/pages/WelcomePage.h \
    src/HaystackConfigWizard.h
SOURCES += src/forms/ipRangeSelectionForm.cpp \
    src/pages/WelcomePage.cpp \
    src/HaystackConfigWizard.cpp \
    src/main.cpp
FORMS += UI/ipRangeSelectionForm.ui
INCLUDEPATH += ../NovaLibrary/src \
../Nova_UI_Core/src
QMAKE_CXXFLAGS += -std=c++0x
CONFIG(debug, debug|release):LIBS += ../NovaLibrary/Debug/libNovaLibrary.a
else:LIBS += ../NovaLibrary/Release/libNovaLibrary.a
CONFIG(debug, debug|release):LIBS += ../Nova_UI_Core/Debug/libNova_UI_Core.so
else:LIBS += ../Nova_UI_Core/Release/libNova_UI_Core.so
LIBS += /usr/lib/libboost_serialization.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libann.a \
    /usr/lib/libaprutil-1.a
UI_DIR = UI_headers/
QMAKE_CLEAN += $(TARGET)
CONFIG += no_keywords
CONFIG += link_pkgconfig
PKGCONFIG += libnotify
PKGCONFIG += libcurl
