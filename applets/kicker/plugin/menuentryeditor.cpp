/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "menuentryeditor.h"

#include <QDir>
#include <QPointer>
#include <QStandardPaths>

#include <KPropertiesDialog>

MenuEntryEditor::MenuEntryEditor(QObject *parent)
    : QObject(parent)
{
}

MenuEntryEditor::~MenuEntryEditor()
{
}

bool MenuEntryEditor::canEdit(const QString &entryPath) const
{
    KFileItemList itemList;
    itemList << KFileItem(QUrl::fromLocalFile(entryPath));

    return KPropertiesDialog::canDisplay(itemList);
}

void MenuEntryEditor::edit(const QString &entryPath, const QString &menuId)
{
    const QString &appsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    const QUrl &entryUrl = QUrl::fromLocalFile(entryPath);

    if (!appsPath.isEmpty() && entryUrl.isValid()) {
        const QDir appsDir(appsPath);
        const QString &fileName = entryUrl.fileName();

        if (appsDir.exists(fileName)) {
            KPropertiesDialog::showDialog(entryUrl, nullptr, false);
        } else {
            if (!appsDir.exists()) {
                if (!QDir::root().mkpath(appsPath)) {
                    return;
                }
            }

            KPropertiesDialog *dialog = new KPropertiesDialog(entryUrl, QUrl::fromLocalFile(appsPath), menuId);
            // KPropertiesDialog deletes itself
            dialog->show();
        }
    }
}
