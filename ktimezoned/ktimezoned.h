/*
   This file is part of the KDE libraries
   Copyright (c) 2007,2009 David Jarvie <djarvie@kde.org>
   Copyright (c) 2013 Martin Klapetek <mklapetek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTIMEZONED_H
#define KTIMEZONED_H

#include "ktimezonedbase.h"

#include <QObject>

class KDirWatch;

class KTimeZoned : public KTimeZonedBase
{
    Q_OBJECT

public:
    explicit KTimeZoned(QObject *parent, const QList<QVariant>&);
    ~KTimeZoned() override;

private Q_SLOTS:
    void updateLocalZone();
    void zonetabChanged();

private:
    void init(bool restart) override;
    bool findZoneTab(const QString &pathFromConfig);

    KDirWatch *m_dirWatch = nullptr;       // watcher for timezone config changes
    KDirWatch *m_zoneTabWatch = nullptr;   // watcher for zone.tab changes
    QString m_zoneinfoDir;       // path to zoneinfo directory
    QString m_zoneTab;           // path to zone.tab file
};

#endif
