#include "dctpinterface.h"
#include "libdctp/dctp.h"

#include <QDebug>

DCTPinterface::DCTPinterface(QObject *parent) : QObject(parent)
{
    socket = init_DCTP_socket(rand()%65534);
}

DCTPinterface::~DCTPinterface()
{
    release_DCTP_socket(socket);
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


QString DCTPinterface::getModuleIp()
{
    return _module_ip;
}

void DCTPinterface::setModuleIp(QString in)
{
    _module_ip = in;
    qDebug() << "set module ip = " + in;
}

QString DCTPinterface::getPassword()
{
    return _password;
}

void DCTPinterface::setPassword(QString in)
{
    _password = in;
    qDebug() << "set password = " + in;
}

int DCTPinterface::tryConnect()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    strncpy(command.name, "dctp_ping", sizeof(command.name));
    qDebug() << QString(command.name);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT);
    return rc;
}

int DCTPinterface::tryAccess()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    strncpy(command.name, "dctp_password", sizeof(command.name));
    strncpy(command.arg,  _password.toUtf8().data(), sizeof(command.arg));

    qDebug() << QString(command.name);
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT);
    return rc;
}

