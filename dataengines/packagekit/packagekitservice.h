/*
    SPDX-FileCopyrightText: 2012 Gregor Taetzner <gregor@freenet.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Plasma/Service>

class PackagekitService : public Plasma::Service
{
    Q_OBJECT
public:
    explicit PackagekitService(QObject *parent = nullptr);
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;
};
