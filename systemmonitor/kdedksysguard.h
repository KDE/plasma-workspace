/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <KDEDModule>
#include <QVariantList>

class KDEDKSysGuard : public KDEDModule
{
    Q_OBJECT

public:
    explicit KDEDKSysGuard(QObject *parent, const QVariantList &);
    ~KDEDKSysGuard() override;

private Q_SLOTS:
    void init();
    void showTaskManager();
};
