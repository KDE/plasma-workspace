/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma5Support/Service>
#include <Plasma5Support/ServiceJob>

using namespace Plasma5Support;

class PowerManagementService : public Plasma5Support::Service
{
    Q_OBJECT

public:
    explicit PowerManagementService(QObject *parent = nullptr);
    ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;
};
