/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Plasma5Support/Service>

class Klipper;

class ClipboardService : public Plasma5Support::Service
{
    Q_OBJECT
public:
    ClipboardService(Klipper *klipper, const QString &uuid);
    ~ClipboardService() override = default;

protected:
    Plasma5Support::ServiceJob *createJob(const QString &operation, QVariantMap &parameters) override;

private:
    Klipper *m_klipper;
    QString m_uuid;
};
