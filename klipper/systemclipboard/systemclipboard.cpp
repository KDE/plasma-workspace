/*
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "systemclipboard.h"

#include "qtclipboard.h"
#include "waylandclipboard.h"

#include <KWindowSystem>
#include <QGuiApplication>

SystemClipboard *SystemClipboard::instance()
{
    if (!qApp || qApp->closingDown()) {
        return nullptr;
    }
    static SystemClipboard *systemClipboard = nullptr;
    if (!systemClipboard) {
        if (KWindowSystem::isPlatformWayland()) {
            systemClipboard = new WaylandClipboard(qApp);
        } else {
            systemClipboard = new QtClipboard(qApp);
        }
    }
    return systemClipboard;
}

SystemClipboard::SystemClipboard(QObject *parent)
    : QObject(parent)
{
}
