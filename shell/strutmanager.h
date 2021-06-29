/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Tranter Madi <trmdi@yandex.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QObject>

class QDBusServiceWatcher;
class ShellCorona;

class StrutManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.PlasmaShell.StrutManager")

public:
    explicit StrutManager(ShellCorona *plasmashellCorona);

    QRect availableScreenRect(int id) const;
    QRegion availableScreenRegion(int id) const;

public Q_SLOTS:
    QRect availableScreenRect(const QString &screenName) const;

    void setAvailableScreenRect(const QString &service, const QString &screenName, const QRect &rect);
    void setAvailableScreenRegion(const QString &service, const QString &screenName, const QList<QRect> &rects);

private:
    ShellCorona *m_plasmashellCorona;

    QDBusServiceWatcher *m_serviceWatcher;
    bool addWatchedService(const QString &service);

    QHash<const QString, QHash<int, QRect>> m_availableScreenRects;
    QHash<const QString, QHash<int, QRegion>> m_availableScreenRegions;
};
