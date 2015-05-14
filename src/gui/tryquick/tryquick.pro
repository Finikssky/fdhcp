TEMPLATE = app

QT += qml quick

SOURCES += main.cpp \
    dctpinterface.cpp

RESOURCES += qml.qrc

LIBS += /home/finiks/diplom/fdhcp/libs/libdyndctp.so
HEADERS += /home/finiks/diplom/fdhcp/include/libdctp/dctp.h \
    dctpinterface.h

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
