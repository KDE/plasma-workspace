/*
    SPDX-FileCopyrightText: 2025 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QUrl>

#include <qqmlregistration.h>

class Utils : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Utils)
    QML_SINGLETON

public:
    Q_INVOKABLE QUrl resolvedFile(const QUrl &url);
};
