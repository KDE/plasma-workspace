/*
    SPDX-FileCopyrightText: 2021 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kcms-common_p.h"

void notifyKcmChange(GlobalChangeType changeType, int arg)
{
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KGlobalSettings"), QStringLiteral("org.kde.KGlobalSettings"), QStringLiteral("notifyChange"));
    message.setArguments({changeType, arg});
    QDBusConnection::sessionBus().send(message);
}
