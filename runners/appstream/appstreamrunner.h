/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <AppStreamQt/pool.h>
#include <KRunner/AbstractRunner>
#include <QMutex>

class InstallerRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    InstallerRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~InstallerRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

private:
    QList<AppStream::Component> findComponentsByString(const QString &query);

    AppStream::Pool m_db;
    QMutex m_appstreamMutex;
};
