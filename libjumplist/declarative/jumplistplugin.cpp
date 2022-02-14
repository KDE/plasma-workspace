/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "jumplistplugin.h"

#include <QQmlEngine>

#include "jumplist.h"

namespace JumpList
{

void JumpListPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.jumplist"));

    qmlRegisterType<JumpListBackend>(uri, 1, 0, "JumpList");
}

}
