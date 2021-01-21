/***************************************************************************
 *   Copyright (C) 2019 by Kai Uwe Broulik <kde@broulik.de>                *
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

    const auto oldRunCommand = KGlobalAccel::self()->globalShortcut(QStringLiteral("krunner"), QStringLiteral("run command"));
    const auto oldRunClipboard = KGlobalAccel::self()->globalShortcut(QStringLiteral("krunner"), QStringLiteral("run command on clipboard contents"));

    // Fake krunner and remove the old shortcuts
    {
        KActionCollection oldCollection(static_cast<QObject *>(nullptr));
        oldCollection.setComponentName(QStringLiteral("krunner"));

        QAction *oldAction = new QAction();
        oldCollection.addAction(QStringLiteral("run command"), oldAction);
        KGlobalAccel::self()->setDefaultShortcut(oldAction, {});
        KGlobalAccel::self()->removeAllShortcuts(oldAction);

        QAction *oldClipboard = new QAction();
        oldCollection.addAction(QStringLiteral("run command on clipboard contents"), oldClipboard);
        KGlobalAccel::self()->setDefaultShortcut(oldClipboard, {});
        KGlobalAccel::self()->removeAllShortcuts(oldClipboard);
    }

    // Fake krunner.desktop launcher and transfer the shortcuts over
    {
        // Since we need to fake those actions, read the translated names from the desktop file
        KDesktopFile df(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/krunner.desktop"));

        QString displayName = QStringLiteral("KRunner");
        if (!df.readName().isEmpty()) {
            displayName = df.readName();
        }

        const QString clipboardActionName =
            df.actionGroup(QStringLiteral("RunClipboard")).readEntry(QStringLiteral("Name"), QStringLiteral("Run command on clipboard contents"));

        KActionCollection shortCutActions(static_cast<QObject *>(nullptr));
        shortCutActions.setComponentName(QStringLiteral("krunner.desktop"));
        shortCutActions.setComponentDisplayName(displayName);

        if (!oldRunCommand.isEmpty()) {
            QAction *runCommandAction = new QAction(displayName);
            shortCutActions.addAction(QStringLiteral("_launch"), runCommandAction);
            KGlobalAccel::self()->setShortcut(runCommandAction, oldRunCommand, KGlobalAccel::NoAutoloading);
        }

        if (!oldRunClipboard.isEmpty()) {
            QAction *runClipboardAction = new QAction(clipboardActionName);
            shortCutActions.addAction(QStringLiteral("RunClipboard"), runClipboardAction);
            KGlobalAccel::self()->setShortcut(runClipboardAction, oldRunClipboard, KGlobalAccel::NoAutoloading);
        }
    }

    return 0;
}
