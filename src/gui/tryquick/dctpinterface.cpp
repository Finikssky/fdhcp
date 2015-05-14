#include "dctpinterface.h"

#include <QDebug>

DCTPinterface::DCTPinterface(QObject *parent) : QObject(parent)
{

}

DCTPinterface::~DCTPinterface()
{

}

QString DCTPinterface::getModule()
{
    return _module;
}

void DCTPinterface::setModule(QString in)
{
    _module = in;
    qDebug() << "set module = " + in;
}
