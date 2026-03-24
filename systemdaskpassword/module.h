/*
    SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDEDModule>

class SystemdAskPasswordModule : public KDEDModule
{
    Q_OBJECT

public:
    SystemdAskPasswordModule(QObject *parent, const QList<QVariant> &);
};
