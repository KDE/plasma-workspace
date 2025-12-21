/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QObject>

class FileDialogNameFilters : public QObject
{
    Q_OBJECT

public:
    explicit FileDialogNameFilters(QObject *parent = nullptr);

    Q_INVOKABLE QString imageFiles() const;
};
