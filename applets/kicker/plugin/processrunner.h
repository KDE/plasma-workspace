/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PROCESSRUNNER_H
#define PROCESSRUNNER_H

#include <QAction>

class ProcessRunner : public QObject
{
    Q_OBJECT

public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    Q_INVOKABLE void runMenuEditor();
};

#endif
