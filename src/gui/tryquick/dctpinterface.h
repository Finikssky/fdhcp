#ifndef DCTPINTERFACE_H
#define DCTPINTERFACE_H

#include <QObject>
//#include "dctp.h"

class DCTPinterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString module READ getModule WRITE setModule NOTIFY moduleChanged)
    Q_PROPERTY(QString module_ip READ getModuleIp WRITE setModuleIp NOTIFY moduleIpChanged)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)

public:
    explicit DCTPinterface(QObject *parent = 0);
    ~DCTPinterface();

signals:
    void moduleChanged();
    void moduleIpChanged();
    void passwordChanged();

public slots:
    QString getModule();
    void setModule(QString);

    QString getModuleIp();
    void setModuleIp(QString);

    QString getPassword();
    void setPassword(QString);

    int tryConnect();
    int tryAccess();

private:
    QString _module;
    QString _module_ip;
    QString _password;
    int socket;
};

#endif // DCTPINTERFACE_H
