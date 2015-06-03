#ifndef DCTPINTERFACE_H
#define DCTPINTERFACE_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
#include "libdctp/dctp.h"

#define TMP_CONFIG_FILE "tmp_config"

class DCTPinterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString module READ getModule WRITE setModule NOTIFY moduleChanged)
    Q_PROPERTY(QString module_ip READ getModuleIp WRITE setModuleIp NOTIFY moduleIpChanged)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString last_error READ getLastError WRITE setLastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QStringList local_config READ getLocal_config WRITE setLocal_config NOTIFY local_configChanged)

public:
    typedef void (DCTPinterface::*DCTPinterfaceFunction) (void);

protected:
    class SingleTask : public QRunnable
    {
        DCTPinterfaceFunction fun;
        DCTPinterface * context;
        public:
            SingleTask(DCTPinterface * CONTEXT, DCTPinterfaceFunction FUN)
            {
                fun = FUN;
                context = CONTEXT;
            }

        virtual void run()
        {
           (context->*fun)();
        }
    };

public:
    explicit DCTPinterface(QObject *parent = 0);
    ~DCTPinterface();

signals:
    void moduleChanged();
    void moduleIpChanged();
    void passwordChanged();
    void connectSuccess();
    void connectFail();
    void accessGranted();
    void accessDenied();
    void lastErrorChanged();
    void configUpdate(QString status);
    void ifaceChangeStateFail(QString signal_arg1);

    void local_configChanged(QStringList arg);

public slots:
    QString getModule();
    void setModule(QString);

    QString getModuleIp();
    void setModuleIp(QString);

    QString getPassword();
    void setPassword(QString);

    QString getLastError();
    void setLastError(QString);

    QStringList getLocal_config();
    void setLocal_config(QStringList);

    void tryConnect();
    void tryAccess();
    void tryUpdateConfig();

    void tryChSubnetProperty(QString, QString, QString, QString, QString);
    int tryChangeInterfaceState(QString, QString);

    QStringList getFullConfig();
    void setFullConfig(QStringList);

    QStringList getIfacesList();
    QStringList getIfaceConfig(QString);
    QString getIfaceState(QString);

    QStringList getSubnets(QString);
    QStringList getSubnetConfig(QString, QString);
    QStringList getSubnetProperty(QString, QString, QString);

    void updateLocalConfig(QStringList chain);

    void doInThread(QString);

private:
    QString _module;
    QString _module_ip;
    QString _password;
    char _last_error[DCTP_ERROR_DESC_SIZE];
    int socket;
    QStringList m_local_config;
};

#endif // DCTPINTERFACE_H
