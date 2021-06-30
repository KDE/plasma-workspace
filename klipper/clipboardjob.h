/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <Plasma/ServiceJob>

class KFileItem;
class Klipper;

class ClipboardJob : public Plasma::ServiceJob
{
    Q_OBJECT
public:
    ClipboardJob(Klipper *klipper, const QString &destination, const QString &operation, const QVariantMap &parameters, QObject *parent = nullptr);
    ~ClipboardJob() override = default;

    void start() override;

private:
    void iconResult(const KFileItem &item);
    Klipper *m_klipper;
};
