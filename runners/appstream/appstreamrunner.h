/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <AppStreamQt/pool.h>
#include <KRunner/AbstractRunner>

class InstallerRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    InstallerRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    QList<AppStream::Component> findComponentsByString(const QString &query);
    AppStream::Pool m_db;
};
