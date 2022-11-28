/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "utils_p.h"

#include "notifications.h"

#include <QAbstractItemModel>
#include <QAbstractProxyModel>
#include <QConcatenateTablesProxyModel>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFile>
#include <QFileInfo>
#include <QMetaEnum>
#include <QSettings>
#include <QTextStream>

#include <KProcessList>

using namespace NotificationManager;

QHash<int, QByteArray> Utils::roleNames()
{
    static QHash<int, QByteArray> s_roles;

    if (s_roles.isEmpty()) {
        // This generates role names from the Roles enum in the form of: FooRole -> foo
        const QMetaEnum e = QMetaEnum::fromType<Notifications::Roles>();

        // Qt built-in roles we use
        s_roles.insert(Qt::DisplayRole, QByteArrayLiteral("display"));
        s_roles.insert(Qt::DecorationRole, QByteArrayLiteral("decoration"));
        s_roles.insert(Qt::AccessibleDescriptionRole, QByteArrayLiteral("accessibleDescription"));

        for (int i = 0; i < e.keyCount(); ++i) {
            const int value = e.value(i);

            QByteArray key(e.key(i));
            key[0] = key[0] + 32; // lower case first letter
            key.chop(4); // strip "Role" suffix

            s_roles.insert(value, key);
        }

        s_roles.insert(Notifications::IdRole, QByteArrayLiteral("notificationId")); // id is QML-reserved
    }

    return s_roles;
}

QString Utils::processNameFromPid(uint pid)
{
    auto processInfo = KProcessList::processInfo(pid);

    if (!processInfo.isValid()) {
        return QString();
    }

    return processInfo.name();
}

QString Utils::desktopEntryFromPid(uint pid)
{
    const QString flatpakInfoPath = QStringLiteral("/proc/%1/root/.flatpak-info").arg(QString::number(pid));
    if (QFileInfo::exists(flatpakInfoPath)) {
        QSettings flatpakInfo(flatpakInfoPath, QSettings::IniFormat);

        const QString name = flatpakInfo.value("Application/name").toString();
        if (!name.isEmpty()) {
            return name;
        }

        // If it's a flatpak, can't be a snap, bail out.
        return QString();
    }

    QFile environFile(QStringLiteral("/proc/%1/environ").arg(QString::number(pid)));
    if (environFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray bamfDesktopFileHint = QByteArrayLiteral("BAMF_DESKTOP_FILE_HINT");

        const auto lines = environFile.readAll().split('\0');
        for (const QByteArray &line : lines) {
            const int equalsIdx = line.indexOf('=');
            if (equalsIdx <= 0) {
                continue;
            }

            const QByteArray key = line.left(equalsIdx);
            if (key == bamfDesktopFileHint) {
                const QByteArray value = line.mid(equalsIdx + 1);
                return value;
            }
        }
    }

    return QString();
}

QModelIndex Utils::mapToModel(const QModelIndex &idx, const QAbstractItemModel *sourceModel)
{
    // KModelIndexProxyMapper can only map different indices to a single source
    // but we have the other way round, a single index that splits into different source models
    QModelIndex resolvedIdx = idx;
    while (resolvedIdx.isValid() && resolvedIdx.model() != sourceModel) {
        if (auto *proxyModel = qobject_cast<const QAbstractProxyModel *>(resolvedIdx.model())) {
            resolvedIdx = proxyModel->mapToSource(resolvedIdx);
            // QConcatenateTablesProxyModel isn't a "real" proxy model, so we need to special case for it :(
        } else if (auto *concatenateModel = qobject_cast<const QConcatenateTablesProxyModel *>(resolvedIdx.model())) {
            resolvedIdx = concatenateModel->mapToSource(resolvedIdx);
        } else {
            if (resolvedIdx.model() != sourceModel) {
                resolvedIdx = QModelIndex(); // give up
            }
        }
    }
    return resolvedIdx;
}

bool Utils::isDBusMaster()
{
    return qApp->property("_plasma_dbus_master").toBool();
}
