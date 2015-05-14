#ifndef DCTPINTERFACE_H
#define DCTPINTERFACE_H

#include <QObject>
//#include "dctp.h"

class DCTPinterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString module READ getModule WRITE setModule NOTIFY moduleChanged)

public:
    explicit DCTPinterface(QObject *parent = 0);
    ~DCTPinterface();

signals:
    void moduleChanged();

public slots:
    QString getModule();
    void setModule(QString);

private:
    QString _module;
};

#endif // DCTPINTERFACE_H
