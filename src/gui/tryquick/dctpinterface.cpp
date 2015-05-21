#include "dctpinterface.h"

#include <QDebug>
#include <QThreadPool>
#include <QRunnable>
#include <QObject>

DCTPinterface::DCTPinterface(QObject *parent) : QObject(parent)
{
    socket = init_DCTP_socket(rand()%65534);
    if (socket == -1) exit(1);
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

QString DCTPinterface::getLastError()
{
    return QString(_last_error);
}

void DCTPinterface::tryConnect()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    command.code = DCTP_PING;
    qDebug() << QString(command.code);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

    qDebug() << "rc = " << rc;

    if (rc != -1)
        emit this->connectSuccess();
    else
        emit this->connectFail();
}

void DCTPinterface::tryUpdateConfig()
{
    qDebug() << "update config";
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    command.code = DCTP_GET_CONFIG;

    qDebug() << QString(command.code);
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);
    qDebug() << "updconf command reply " << rc;
    if (rc == -1) return;

    rc = receive_DCTP_CONFIG(socket, TMP_CONFIG_FILE);
    qDebug() << "receive configuration status " << rc;
    if (rc != -1)
        emit this->configUpdateSuccess();
    else
        emit this->configUpdateFail();

}

void DCTPinterface::tryAccess()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    command.code = DCTP_PASSWORD;
    strncpy(command.arg,  _password.toUtf8().data(), sizeof(command.arg));

    qDebug() << QString(command.code);
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

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
    else if (fname == "tryUpdateConfig") task = new SingleTask(this, &DCTPinterface::tryUpdateConfig);
    else return;

    QThreadPool::globalInstance()->start(task);
}



