TEMPLATE = app
TARGET = AutoConfigWizard
QT += core \
    gui
HEADERS += src/autoconfigwizard.h
SOURCES += src/main.cpp \
    src/autoconfigwizard.cpp
FORMS += UI/autoconfigwizard.ui
RESOURCES += 
UI_DIR = UI_headers/
QMAKE_CLEAN += $(TARGET)
