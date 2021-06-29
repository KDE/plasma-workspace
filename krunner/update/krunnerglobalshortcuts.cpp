/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>

#include <KActionCollection>
#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <KGlobalAccel>
#include <KSharedConfig>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const QString oldComponentName = QStringLiteral("krunner");
    const QString oldDesktopFile = QStringLiteral("krunner.desktop");
    const QString newDesktopFile = QStringLiteral("org.kde.krunner.desktop");

    // Since we need to fake those actions, read the translated names from the desktop file
    KDesktopFile df(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/") + newDesktopFile);
    QString displayName = QStringLiteral("KRunner");
    if (!df.readName().isEmpty()) {
        displayName = df.readName();
    }
    const QString clipboardActionName = df.actionGroup(QStringLiteral("RunClipboard"))
                                            .readEntry(QStringLiteral("Name"), //
                                                       QStringLiteral("Run command on clipboard contents"));

    KActionCollection shortCutActions(nullptr, newDesktopFile);
    shortCutActions.setComponentDisplayName(displayName);
    // The actions are intentionally allocated and never cleaned up, because otherwise KGlobalAccel
    // will mark them as inactive
    auto runCommandAction = new QAction(displayName);
    shortCutActions.addAction(QStringLiteral("_launch"), runCommandAction);
    auto runClipboardAction = new QAction(clipboardActionName);
    shortCutActions.addAction(QStringLiteral("RunClipboard"), runClipboardAction);

    QList<QKeySequence> oldRunCommand;
    QList<QKeySequence> oldRunClipboard;

    // It can happen that the old component is not active so we do it unconditionally
    KActionCollection oldActions(nullptr, oldComponentName);
    QAction oldRunCommandAction, oldRunClipboardAction;
    oldActions.addAction(QStringLiteral("run command"), &oldRunCommandAction);
    oldActions.addAction(QStringLiteral("run command on clipboard contents"), &oldRunClipboardAction);
    oldRunCommand = KGlobalAccel::self()->globalShortcut(oldComponentName, oldRunCommandAction.objectName());
    oldRunClipboard = KGlobalAccel::self()->globalShortcut(oldComponentName, oldRunClipboardAction.objectName());
    KGlobalAccel::self()->setShortcut(&oldRunCommandAction, {});
    KGlobalAccel::self()->setShortcut(&oldRunClipboardAction, {});
    KGlobalAccel::self()->removeAllShortcuts(&oldRunCommandAction);
    KGlobalAccel::self()->removeAllShortcuts(&oldRunClipboardAction);
    KGlobalAccel::self()->cleanComponent(oldComponentName);

    if (KGlobalAccel::isComponentActive(oldDesktopFile)) {
        oldRunCommand = KGlobalAccel::self()->globalShortcut(oldDesktopFile, runCommandAction->objectName());
        oldRunClipboard = KGlobalAccel::self()->globalShortcut(oldDesktopFile, runClipboardAction->objectName());
        KGlobalAccel::self()->cleanComponent(oldDesktopFile);
    }

    if (!oldRunCommand.isEmpty()) {
        KGlobalAccel::self()->setShortcut(runCommandAction, oldRunCommand, KGlobalAccel::NoAutoloading);
    }
    if (!oldRunClipboard.isEmpty()) {
        KGlobalAccel::self()->setShortcut(runClipboardAction, oldRunClipboard, KGlobalAccel::NoAutoloading);
    }

    return 0;
}
