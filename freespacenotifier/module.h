/*
    SPDX-FileCopyrightText: 2006 Lukas Tinkl <ltinkl@suse.cz>
    SPDX-FileCopyrightText: 2008 Lubos Lunak <l.lunak@suse.cz>
    SPDX-FileCopyrightText: 2009 Ivo Anjo <knuckles@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDEDModule>
#include <QObject>

#include <Solid/StorageAccess>

#include "freespacenotifier.h"

class FreeSpaceNotifierModule : public KDEDModule
{
    Q_OBJECT
public:
    FreeSpaceNotifierModule(QObject *parent, const QList<QVariant> &);

private:
    void showConfiguration();
    void onNewSolidDevice(const QString &udi);
    void startTracking(const QString &udi, Solid::StorageAccess *access);
    void stopTracking(const QString &udi);

    QMap<QString, FreeSpaceNotifier *> m_notifiers;
};
