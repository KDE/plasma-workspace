/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class MenuEntryEditor : public QObject
{
    Q_OBJECT

public:
    explicit MenuEntryEditor(QObject *parent = nullptr);
    ~MenuEntryEditor() override;

    bool canEdit(const QString &entryPath) const;

public Q_SLOTS:
    void edit(const QString &entryPath, const QString &menuId);
};
