TEMPLATE = app

QT += qml quick

SOURCES += main.cpp \
    dctpinterface.cpp

RESOURCES += qml.qrc

INCLUDEPATH += /home/finiks/diplom/fdhcp/include
QMAKE_LIBDIR += /home/finiks/diplom/fdhcp/libs
LIBS += -ldyndctp

HEADERS += dctpinterface.h


# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
