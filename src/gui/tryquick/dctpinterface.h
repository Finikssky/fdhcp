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
    Q_PROPERTY(QString last_error READ getLastError)

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
           qDebug() << "ha!";
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
    void configUpdateSuccess();
    void configUpdateFail();
    void ifaceChangeStateFail(QString signal_arg1);

public slots:
    QString getModule();
    void setModule(QString);

    QString getModuleIp();
    void setModuleIp(QString);

    QString getPassword();
    void setPassword(QString);

    QString getLastError();

    void tryConnect();
    void tryAccess();
    void tryUpdateConfig();
    int tryChangeInterfaceState(QString, QString);

    QStringList getIfacesList();
    QString getIfaceState(QString);

    void doInThread(QString);

private:
    QString _module;
    QString _module_ip;
    QString _password;
    char _last_error[DCTP_ERROR_DESC_SIZE];
    int socket;
};

#endif // DCTPINTERFACE_H
