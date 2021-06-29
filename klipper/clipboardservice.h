/*
This file is part of the KDE project.

SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KLIPPER_CLIPBOARDSERVICE_H
#define KLIPPER_CLIPBOARDSERVICE_H

#include <Plasma/Service>

class Klipper;

class ClipboardService : public Plasma::Service
{
    Q_OBJECT
public:
    ClipboardService(Klipper *klipper, const QString &uuid);
    ~ClipboardService() override = default;

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QVariantMap &parameters) override;

private:
    Klipper *m_klipper;
    QString m_uuid;
};

#endif
