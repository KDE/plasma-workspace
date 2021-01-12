#ifndef LAYOUTNAMES_H
#define LAYOUTNAMES_H

#include <QMetaType>

class QDBusArgument;

class LayoutNames
{
    Q_GADGET

    Q_PROPERTY(QString shortName MEMBER shortName)
    Q_PROPERTY(QString displayName MEMBER displayName)
    Q_PROPERTY(QString longName MEMBER longName)

public:
    static void registerMetaType();

    QString shortName;
    QString displayName;
    QString longName;
};
Q_DECLARE_METATYPE(LayoutNames)

QDBusArgument &operator<<(QDBusArgument &argument, const LayoutNames &layoutNames);
const QDBusArgument &operator>>(const QDBusArgument &argument, LayoutNames &layoutNames);

#endif // LAYOUTNAMES_H
