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
    m_local_config.clear();
    socket = init_DCTP_socket(rand()%65534);
    if (socket == -1) exit(1);
}

DCTPinterface::~DCTPinterface()
{
    release_DCTP_socket(socket);
}

QStringList DCTPinterface::getLocal_config()
{
    return m_local_config;
}

void DCTPinterface::setLocal_config(QStringList arg)
{
    if (m_local_config == arg)
        return;

    m_local_config = arg;
    emit local_configChanged(arg);
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

void DCTPinterface::setLastError(QString value)
{
    strncpy(_last_error, value.toUtf8().data(), sizeof(_last_error));
    qDebug() << "set LE = " + value;
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
    {
        emit this->connectFail();
        emit this->lastErrorChanged();
    }
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
    {
        emit this->configUpdate("fail");
        emit this->lastErrorChanged();
    }

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

    qDebug() << _last_error;

    if (rc != -1)
        emit this->accessGranted();
    else
    {
        emit this->accessDenied();
        emit this->lastErrorChanged();
    }

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

    if (rc == -1)
    {
        emit ifaceChangeStateFail(name);
        emit lastErrorChanged();
    }

    return rc;
}

void DCTPinterface::tryChSubnetProperty(QString work, QString iface, QString subnet, QString option, QString arg)
{
    DCTP_COMMAND command;
    memset(&command, 0, sizeof(DCTP_COMMAND));
    iface = iface.replace("  ", " ").replace("\n", "").trimmed();
    subnet = subnet.replace("  ", " ").replace("\n", "").trimmed();
    arg = arg.replace("  ", " ").replace("\n", "").trimmed();

    QStringList chain;
    chain.append("interface");
    chain.append(iface);
    chain.append("subnet");
    chain.append(subnet);
    chain.append("opt");
    chain.append(option);
    chain.append("opt_work");
    chain.append(work);
    chain.append("args");
    chain.append(arg);


    if (_module == "server")
    {
        if (work == "add")
        {
             if (option == "name")
             {
                command.code = SR_ADD_SUBNET;
             }
             else if (option == "range")
             {
                command.code = SR_ADD_POOL;
             }
             else if (option == "dns-server")
             {
                command.code = SR_ADD_DNS;
             }
             else if (option == "router")
             {
                command.code = SR_ADD_ROUTER;
             }
             else if (option == "domain_name")
             {
                command.code = SR_SET_DOMAIN_NAME;
             }
             else if (option == "lease_time")
             {
                 command.code = SR_SET_LEASETIME;
             }
        }
        else
        {
            if (option == "name")
            {
               command.code = SR_DEL_SUBNET;
            }
            else if (option == "range")
            {
               command.code = SR_DEL_POOL;
            }
            else if (option == "dns-server")
            {
               command.code = SR_DEL_DNS;
            }
            else if (option == "router")
            {
               command.code = SR_DEL_ROUTER;
            }
            else if (option == "domain_name")
            {
               command.code = SR_SET_DOMAIN_NAME;
               arg = "";
            }
            else if (option == "lease_time")
            {
                command.code = SR_SET_LEASETIME;
                arg = "60";
            }
        }
    }
    strncpy(command.interface, iface.toUtf8().data(), sizeof(command.interface));
    snprintf(command.arg, sizeof(command.arg), "%s%s%s", subnet.toUtf8().data(), (arg.length() != 0) ? " " : "", arg.toUtf8().data());

    qDebug() << QString(command.interface);
    qDebug() << QString(stringize_DCTP_COMMAND_CODE(command.code));
    qDebug() << QString(command.arg);

    int rc = send_DCTP_COMMAND(socket, command, _module_ip.toUtf8().data(), _module == "server" ? DSR_DCTP_PORT : DCL_DCTP_PORT, _last_error);

    if (rc == -1)
        emit lastErrorChanged();
    else
        updateLocalConfig(chain);
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

QStringList DCTPinterface::getFullConfig()
{
    if (m_local_config.count() != 0) return m_local_config;

    m_local_config.clear();

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
         m_local_config.append(f.readLine(512).replace("\n", "").replace("  ", " "));
    }
    f.close();

    //qDebug() << ret;
    return m_local_config;
}

void DCTPinterface::setFullConfig(QStringList in)
{
    QFile f(TMP_CONFIG_FILE);

    if (f.exists())
    {
        if (!f.open(QIODevice::WriteOnly))
        {
            qDebug() << "Ошибка открытия для записи";
        }
    }

    for (int i = 0; i < in.length(); i++)
    {
        QString str = in[i] + "\n";
        QByteArray raw = str.toUtf8();
        f.write(raw, raw.length());
    }

    f.close();
}

QStringList DCTPinterface::getIfacesList()
{
    QStringList config = getFullConfig();
    QStringList ret;

    for (int i = 0; i < config.count(); i++)
    {
        QString line = config[i];

        if (line.contains("interface:"))
        {
              QString raw_iface_name = line.section(':', 1);
              QString clear_iface_name = raw_iface_name.trimmed();
              if (!ret.contains(clear_iface_name)) ret.append(clear_iface_name);
        }
    }

    return ret;
}

QStringList DCTPinterface::getIfaceConfig(QString name)
{
    QStringList config = getFullConfig();
    QStringList ret;

    for (int i = 0; i < config.count(); i++)
    {
        QString line = config[i];

        if (line.contains("interface:") && line.contains(name))
        {
              while (1)
              {
                  QString iface_line = config[i];
                  if (iface_line.contains("end_interface")) return ret;

                  ret.append(iface_line);
                  i++;
              }
        }
    }

    return ret;
}

QString DCTPinterface::getIfaceState(QString name)
{
    QStringList if_config = getIfaceConfig(name);
    for (int i = 0; i < if_config.count(); i++)
    {
        if (if_config[i].contains("enable")) return "on";
        if (if_config[i].contains("disable")) return "off";
    }

    return "off";
}

QStringList DCTPinterface::getSubnetConfig(QString if_name, QString sub_name)
{
    QStringList if_config = getIfaceConfig(if_name);
    QStringList ret;

    for (int i = 0; i < if_config.count(); i++)
    {
        QString line = if_config[i];

        if (line.contains("subnet:") && line.contains(sub_name))
        {
              while (1)
              {
                  QString subnet_line = if_config[i];
                  if (subnet_line.contains("end_subnet")) return ret;

                  ret.append(subnet_line);
                  i++;
              }
        }
    }

    return ret;
}

QStringList DCTPinterface::getSubnets(QString name)
{
    QStringList if_config;
    QStringList ret;

    if_config = getIfaceConfig(name);

    for (int i = 0; i < if_config.count(); i++)
    {
        QString line = if_config[i];
        if (line.contains("subnet:"))
        {
            QString raw_subnet_name = line.section(':', 1);
            QString clear_subnet_name = raw_subnet_name.trimmed();
            if (!ret.contains(clear_subnet_name)) ret.append(clear_subnet_name);
        }
    }

    return ret;
}

QStringList DCTPinterface::getSubnetProperty(QString if_name, QString sub_name, QString property_name)
{
    QStringList sub_config = getSubnetConfig(if_name, sub_name);
    QStringList ret;

    for (int i = 0; i < sub_config.count(); i++)
    {
        QString line = sub_config[i];
        if (line.contains(property_name))
        {
            QString raw_opt_arg = line.section(':', 1);
            QString clear_opt_arg = raw_opt_arg.trimmed();
            if (!ret.contains(clear_opt_arg)) ret.append(clear_opt_arg);
        }
    }

    return ret;
}

void DCTPinterface::updateLocalConfig(QStringList chain)
{
    QStringList new_config;

    int idx = 1;
    QString iface;
    QString subnet;
    QString opt;
    QString opt_work;
    QString args;
    if (chain.contains("interface")) { iface = chain[idx]; idx+=2; }
    if (chain.contains("subnet")) { subnet = chain[idx]; idx+=2; }
    if (chain.contains("opt")) { opt = chain[idx]; idx += 2;}
    if (chain.contains("opt_work")) { opt_work = chain[idx]; idx+=2; }
    if (chain.contains("args")) { args = chain[idx]; }

    qDebug() << "chain=" << chain;
    qDebug() << "values=" << iface << "," << subnet << "," << opt << "," << opt_work << "," << args;

    QStringList ifaces = getIfacesList();
    for (int if_idx = 0; if_idx < ifaces.length(); if_idx++)
    {
        QStringList iface_config = getIfaceConfig(ifaces[if_idx]);
        if (ifaces[if_idx] != iface)
            new_config.append(iface_config);
        else
        {
            new_config.append(("interface: " + iface));
            new_config.append(getIfaceState(iface) == "off" ? "  disable" : "    enable");

            QStringList subnets = getSubnets(iface);
            for (int sub_idx = 0; sub_idx < subnets.length(); sub_idx++)
            {
                QStringList subnet_config = getSubnetConfig(iface, subnets[sub_idx]);
                if (subnets[sub_idx] != subnet)
                    new_config.append(subnet_config);
                else
                {
                    if (opt_work == "del" && opt == "") continue;

                    new_config.append(("    subnet: " + subnet));
                    for (int opt_idx = 1; opt_idx < (subnet_config.length() - 1); opt_idx++)
                    {
                        if (subnet_config[opt_idx].contains(opt) && subnet_config[opt_idx].contains(args) && opt_work == "del") continue;
                        new_config.append(subnet_config[opt_idx]);
                    }
                    if (opt_work == "add") new_config.append(("     " + opt + ": " + args));
                    new_config.append(("    end_subnet"));
                }
            }

            if (opt_work == "add" && opt == "" && subnet != "")
            {
                new_config.append(("    subnet: " + subnet));
                new_config.append(("    end_subnet"));
            }

            new_config.append("end_interface");
        }
    }

    qDebug() << new_config;
    m_local_config = new_config;
    setFullConfig(new_config);
}



