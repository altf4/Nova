TEMPLATE = app
TARGET = haystackconfigwizard
QT += core \
    gui
HEADERS += src/pages/NetworkSelectionPage.h \
    src/HaystackConfigWizard.h
SOURCES += src/pages/NetworkSelectionPage.cpp \
    src/HaystackConfigWizard.cpp \
    src/main.cpp
FORMS += 
RESOURCES += 
UI_DIR = UI_headers/
QMAKE_CLEAN += $(TARGET)
