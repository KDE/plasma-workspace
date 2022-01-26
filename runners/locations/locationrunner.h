/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <krunner/abstractrunner.h>

class LocationsRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    LocationsRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~LocationsRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;
};
