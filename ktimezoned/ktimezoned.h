/*
    SPDX-FileCopyrightText: 2007, 2009 David Jarvie <djarvie@kde.org>
    SPDX-FileCopyrightText: 2013 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "ktimezonedbase.h"

#include <QObject>

class KDirWatch;

class KTimeZoned : public KTimeZonedBase
{
    Q_OBJECT

public:
    explicit KTimeZoned(QObject *parent, const QList<QVariant> &);
    ~KTimeZoned() override;

private Q_SLOTS:
    void updateLocalZone();
    void zonetabChanged();

private:
    void init(bool restart) override;
    bool findZoneTab(const QString &pathFromConfig);

    KDirWatch *m_dirWatch = nullptr; // watcher for timezone config changes
    KDirWatch *m_zoneTabWatch = nullptr; // watcher for zone.tab changes
    QString m_zoneinfoDir; // path to zoneinfo directory
    QString m_zoneTab; // path to zone.tab file
};
