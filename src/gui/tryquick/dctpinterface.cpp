#include "dctpinterface.h"
#include "libdctp/dctp.h"

#include <QDebug>
#include <QThreadPool>
#include <QRunnable>
#include <QObject>

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

void DCTPinterface::tryConnect()
{
    qDebug() << "Hello world from thread" << QThread::currentThread();
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    strncpy(command.name, "dctp_ping", sizeof(command.name));
    qDebug() << QString(command.name);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT);

    qDebug() << "rc = " << rc;

    if (rc != -1)
        emit this->connectSuccess();
    else
        emit this->connectFail();
}

void DCTPinterface::tryAccess()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    strncpy(command.name, "dctp_password", sizeof(command.name));
    strncpy(command.arg,  _password.toUtf8().data(), sizeof(command.arg));

    qDebug() << QString(command.name);
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT);

    if (rc != -1)
        emit this->accessGranted();
    else
        emit this->accessDenied();

}

void DCTPinterface::doInThread(QString fname)
{
    SingleTask * task;
    if (fname == "tryConnect") task = new SingleTask(this, &DCTPinterface::tryConnect);
    else if (fname == "tryAccess") task = new SingleTask(this, &DCTPinterface::tryAccess);
    else return;

    QThreadPool::globalInstance()->start(task);
}



