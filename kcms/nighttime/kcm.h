/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickManagedConfigModule>

class NightTimeSettings;
class NightTimeData;

class KCMNightTime : public KQuickManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(NightTimeSettings *settings READ settings CONSTANT)

public:
    KCMNightTime(QObject *parent, const KPluginMetaData &data);

    NightTimeSettings *settings() const;

    // Drop it when https://bugreports.qt.io/browse/QTBUG-139933 gets fixed.
    Q_INVOKABLE QString escapeRegExp(const QString &string) const;

private:
    NightTimeData *m_data;
};
