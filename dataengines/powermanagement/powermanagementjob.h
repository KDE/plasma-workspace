/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef POWERMANAGEMENTJOB_H
#define POWERMANAGEMENTJOB_H

// plasma
#include <Plasma/ServiceJob>

class SessionManagement;
class QDBusPendingCall;

class PowerManagementJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr);
    ~PowerManagementJob() override;

protected:
    void start() override;

private:
    QDBusPendingCall setScreenBrightness(int value, bool silent);
    QDBusPendingCall setKeyboardBrightness(int value, bool silent);
    SessionManagement *m_session;
};

#endif // POWERMANAGEMENTJOB_H
