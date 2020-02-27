/*
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Tranter Madi <trmdi@yandex.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef STRUTMANAGER_H
#define STRUTMANAGER_H

#include <QObject>
#include <QHash>

class QDBusServiceWatcher;
class ShellCorona;

class StrutManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","org.kde.PlasmaShell.StrutManager")

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

        QHash <const QString, QHash<int, QRect>> m_availableScreenRects;
        QHash <const QString, QHash<int, QRegion>> m_availableScreenRegions;
};

#endif
