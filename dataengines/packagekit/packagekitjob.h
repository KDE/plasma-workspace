/*
    SPDX-FileCopyrightText: 2012 Gregor Taetzner <gregor@freenet.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Plasma/ServiceJob>

class PackagekitJob : public Plasma::ServiceJob
{
    Q_OBJECT
public:
    PackagekitJob(const QString &destination, const QString &operation, const QMap<QString, QVariant> &parameters, QObject *parent = nullptr);
    ~PackagekitJob() override;

protected:
    void start() override;
};
