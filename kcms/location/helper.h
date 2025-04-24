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
    KAuth::ActionReply reconfigure(const QVariantMap &parameters);
    KAuth::ActionReply query(const QVariantMap &parameters);
    KAuth::ActionReply enable(const QVariantMap &parameters);
    KAuth::ActionReply disable(const QVariantMap &parameters);
    KAuth::ActionReply setstaticlocation(const QVariantMap &parameters);
    KAuth::ActionReply unsetstaticlocation(const QVariantMap &parameters);
};
