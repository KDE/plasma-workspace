/*
    SPDX-FileCopyrightText: 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <KDEDModule>

class Configurator : public KDEDModule
{
    Q_OBJECT

public:
    Configurator(QObject *parent, const QList<QVariant> &args);

public Q_SLOTS:
    void onNotifyChange(int type, int arg);
    void onRefreshFonts();

private:
    KConfigWatcher::Ptr m_kwinrcWatcher;
};
