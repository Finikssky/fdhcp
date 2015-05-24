#include "dctpinterface.h"

#include <QDebug>
#include <QThreadPool>
#include <QRunnable>
#include <QObject>
#include <QFile>
#include <QQuickView>
#include <QQmlContext>

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
    qDebug() << QString(stringize_DCTP_COMMAND_CODE(command.code));

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

    qDebug() << "rc = " << rc;

    if (rc != -1)
        emit this->connectSuccess();
    else
        emit this->connectFail();
}

void DCTPinterface::tryUpdateConfig()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    command.code = DCTP_GET_CONFIG;

    qDebug() << QString(stringize_DCTP_COMMAND_CODE(command.code));
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);
    qDebug() << "updconf command reply " << rc;
    if (rc == -1) return;

    rc = receive_DCTP_CONFIG(socket, TMP_CONFIG_FILE);
    qDebug() << "receive configuration status " << rc;
    if (rc != -1)
        emit this->configUpdate("success");
    else
        emit this->configUpdate("fail");

}

void DCTPinterface::tryAccess()
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    command.code = DCTP_PASSWORD;
    strncpy(command.arg,  _password.toUtf8().data(), sizeof(command.arg));

    qDebug() << QString(stringize_DCTP_COMMAND_CODE(command.code));
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

    if (rc != -1)
        emit this->accessGranted();
    else
        emit this->accessDenied();

}

int DCTPinterface::tryChangeInterfaceState(QString name, QString state)
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));

    if (_module == "server")
    {
        if (state == "on")
            command.code = SR_SET_IFACE_ENABLE;
        else
            command.code = SR_SET_IFACE_DISABLE;
    }
    else
    {
        if (state == "on")
            command.code = CL_SET_IFACE_ENABLE;
        else
            command.code = CL_SET_IFACE_DISABLE;
    }

    strncpy(command.interface, name.toUtf8(), sizeof(command.interface));

    qDebug() << QString(stringize_DCTP_COMMAND_CODE(command.code));
    qDebug() << QString(command.interface);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

    if (rc == -1) emit ifaceChangeStateFail(name);

    return rc;
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

QStringList DCTPinterface::getIfacesList()
{
     QStringList ret;
     QFile f(TMP_CONFIG_FILE);

     if (f.exists())
     {
         if (!f.open(QIODevice::ReadOnly))
         {
             qDebug() << "Ошибка открытия для чтения";
         }
     }

     while (!f.atEnd())
     {
          QString line = f.readLine(512);
          if (line.contains("interface:"))
          {
                QString raw_iface_name = line.section(':', 1);
                QString clear_iface_name = raw_iface_name.replace(" ","").replace("\n", "");
                if (!ret.contains(clear_iface_name)) ret.append(clear_iface_name);
          }

     }

     return ret;
}

QString DCTPinterface::getIfaceState(QString name)
{
     QFile f(TMP_CONFIG_FILE);

     if (f.exists())
     {
         if (!f.open(QIODevice::ReadOnly))
         {
             qDebug() << "Ошибка открытия для чтения";
         }
     }

     while (!f.atEnd())
     {
          QString line = f.readLine(512);
          if (line.contains("interface:") && line.contains(name))
          {
                while (1)
                {
                    QString iface_line = f.readLine(512);
                    if (iface_line.contains("enable")) return "on";
                    if (iface_line.contains("disable")) return "off";
                    if (iface_line.contains("end_interface")) return "off";
                }
          }

     }

     return "off";
}



