/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <qtypes.h>

class InhibitMonitor : public QObject
{
    Q_OBJECT
public:
    explicit InhibitMonitor();
    ~InhibitMonitor() override;

    static InhibitMonitor &self();

    bool getInhibit();

    void inhibit(const QString &reason, bool isSilent);
    void uninhibit(bool isSilent);

private:
    void beginSuppressingSleep(const QString &reason, bool isSilent);
    void stopSuppressingSleep(bool isSilent);
    void beginSuppressingScreenPowerManagement(const QString &reason);
    void stopSuppressingScreenPowerManagement();

Q_SIGNALS:
    void isManuallyInhibitedChanged(bool status);
    void isManuallyInhibitedChangeError(bool status);

private:
    std::optional<uint> m_sleepInhibitionCookie;
    std::optional<uint> m_lockInhibitionCookie;
};
