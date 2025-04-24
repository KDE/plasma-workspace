/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <KAuth/ActionReply>

class Helper : public QObject
{
    Q_OBJECT

public:
    Helper();

public Q_SLOTS:
    KAuth::ActionReply getlocation(const QVariantMap &parameters);
    KAuth::ActionReply setlocation(const QVariantMap &parameters);
    KAuth::ActionReply unsetlocation(const QVariantMap &parameters);
};
