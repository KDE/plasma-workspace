/*
    SPDX-FileCopyrightText: 2026 Tomáš Hnyk <tomashnyk@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KGlobalAccel>

#include <QAction>
#include <QGuiApplication>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    const QString component = u"plasmashell"_s;
    const QString actionName = u"clipboard_action"_s;

    // Remove Meta+Ctrl+X (both from default and custom fields), keep other shortcuts untouched.
    QList<QKeySequence> shortcuts = KGlobalAccel::self()->globalShortcut(component, actionName);
    if (shortcuts.removeAll(QKeySequence(Qt::META | Qt::CTRL | Qt::Key_X)) == 0) {
        return 0;
    }

    QAction action;
    action.setObjectName(actionName);
    action.setProperty("componentName", component);
    action.setProperty("componentDisplayName", component);
    KGlobalAccel::self()->setShortcut(&action, shortcuts, KGlobalAccel::NoAutoloading);

    return 0;
}
