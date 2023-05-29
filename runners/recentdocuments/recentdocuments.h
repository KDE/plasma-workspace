/*
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <krunner/abstractrunner.h>

#include <QAction>
#include <QIcon>

class RecentDocuments : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    RecentDocuments(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private:
    QList<QAction *> m_actions;
};
