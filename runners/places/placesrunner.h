/*
    SPDX-FileCopyrightText: 2008 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KFilePlacesModel>
#include <KRunner/AbstractRunner>

class PlacesRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    PlacesRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;
    void init() override;

private:
    Q_INVOKABLE void openDevice(const QString &udi);
    std::unique_ptr<KFilePlacesModel> m_places;
    QString m_pendingUdi;
};
