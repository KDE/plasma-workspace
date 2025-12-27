#pragma once

#include <QMetaType>
#include <qqmlregistration.h>

class QDBusArgument;

class LayoutNames
{
    Q_GADGET
    QML_VALUE_TYPE(layoutNames)

    Q_PROPERTY(QString shortName MEMBER shortName)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(QString longName MEMBER longName)

public:
    static void registerMetaType();

    QString shortName;
    QString displayName;
    QString longName;
};

QDBusArgument &operator<<(QDBusArgument &argument, const LayoutNames &layoutNames);
const QDBusArgument &operator>>(const QDBusArgument &argument, LayoutNames &layoutNames);
