/***************************************************************************
 *   Copyright (C) 2019 by Kai Uwe Broulik <kde@broulik.de>                *
 *   Copyright (C) 2020 David Redondo <kde@david-redondo.de>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

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

    const QString oldCompomentName = QStringLiteral("krunner");
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

    KActionCollection shortCutActions(nullptr, oldDesktopFile);
    shortCutActions.setComponentDisplayName(displayName);
    // The actions are intentionally allocated and never cleaned up, because otherwise KGlobalAccel
    // will mark them as inactive
    auto runCommandAction = new QAction(displayName);
    shortCutActions.addAction(QStringLiteral("_launch"), runCommandAction);
    auto runClipboardAction = new QAction(clipboardActionName);
    shortCutActions.addAction(QStringLiteral("RunClipboard"), runClipboardAction);

    QList<QKeySequence> oldRunCommand;
    QList<QKeySequence> oldRunClipboard;
    if (KGlobalAccel::isComponentActive(oldCompomentName)) {
        oldRunCommand = KGlobalAccel::self()->globalShortcut(oldCompomentName, QStringLiteral("run command"));
        oldRunClipboard = KGlobalAccel::self()->globalShortcut(oldCompomentName, QStringLiteral("run command on clipboard contents"));
        KGlobalAccel::self()->cleanComponent(oldCompomentName);
    } else if (KGlobalAccel::isComponentActive(oldDesktopFile)) {
        oldRunCommand = KGlobalAccel::self()->globalShortcut(oldDesktopFile, runCommandAction->objectName());
        oldRunClipboard = KGlobalAccel::self()->globalShortcut(oldDesktopFile, runClipboardAction->objectName());
        KGlobalAccel::self()->cleanComponent(oldDesktopFile);
    } else {
        return 0;
    }

    shortCutActions.takeAction(runCommandAction);
    shortCutActions.takeAction(runClipboardAction);
    shortCutActions.setComponentName(newDesktopFile);
    shortCutActions.addActions({runCommandAction, runClipboardAction});

    if (!oldRunCommand.isEmpty()) {
        KGlobalAccel::self()->setShortcut(runCommandAction, oldRunCommand, KGlobalAccel::NoAutoloading);
    }
    if (!oldRunClipboard.isEmpty()) {
        KGlobalAccel::self()->setShortcut(runClipboardAction, oldRunClipboard, KGlobalAccel::NoAutoloading);
    }

    return 0;
}
