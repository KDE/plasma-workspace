/*
    SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KGlobalAccel>

#include <QAction>
#include <QGuiApplication>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QAction action;
    action.setObjectName("stop current activity");
    action.setProperty("componentName", QStringLiteral("plasmashell"));
    action.setProperty("componentDisplayName", QStringLiteral("plasmashell"));
    KGlobalAccel::self()->setShortcut(&action, {QKeySequence()}, KGlobalAccel::NoAutoloading);
    KGlobalAccel::self()->removeAllShortcuts(&action);

    return 0;
}
