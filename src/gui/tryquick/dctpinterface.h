#ifndef DCTPINTERFACE_H
#define DCTPINTERFACE_H

#include <QObject>
#include <QRunnable>
#include <QDebug>
//#include "dctp.h"

#define TMP_CONFIG_FILE "tmp_config"

class DCTPinterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString module READ getModule WRITE setModule NOTIFY moduleChanged)
    Q_PROPERTY(QString module_ip READ getModuleIp WRITE setModuleIp NOTIFY moduleIpChanged)
    Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)

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

public slots:
    QString getModule();
    void setModule(QString);

    QString getModuleIp();
    void setModuleIp(QString);

    QString getPassword();
    void setPassword(QString);

    void tryConnect();
    void tryAccess();
    void tryUpdateConfig();

    void doInThread(QString);

private:
    QString _module;
    QString _module_ip;
    QString _password;
    int socket;
};

#endif // DCTPINTERFACE_H
