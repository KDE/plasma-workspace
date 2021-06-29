/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>

    SPDX-License-Identifier: LGPL-2.1-or-later

*/

#ifndef KDEDKSYSGUARD_H
#define KDEDKSYSGUARD_H

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

#endif // KDEDKSYSGUARD_H
