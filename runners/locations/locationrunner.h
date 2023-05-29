/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <krunner/abstractrunner.h>

class LocationsRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    LocationsRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;
};
