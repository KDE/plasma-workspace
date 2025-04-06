/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAction>

#include <qqmlregistration.h>

class ProcessRunner : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    Q_INVOKABLE static void runMenuEditor(QString arg = QString());
};
