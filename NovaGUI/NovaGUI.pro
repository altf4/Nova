TEMPLATE = app
TARGET = NovaGUI
QT += core \
    gui
HEADERS += novagui.h
SOURCES += main.cpp \
    novagui.cpp
FORMS += novagui.ui
RESOURCES += 
INCLUDEPATH += $$HOME/Code/NovaGit/Nova/NovaLibrary/src
LIBS += $$HOME/Code/NovaGit/Nova/NovaLibrary/Debug/libNovaLibrary.a \
    /usr/lib/libboost_serialization.a \
    /usr/lib/libann.a \
    /usr/lib/liblog4cxx.a \
    /usr/lib/libapr-1.a \
    /usr/lib/libaprutil-1.a