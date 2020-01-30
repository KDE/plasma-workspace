/*
 * Copyright 2011 Sebastian Kügler <sebas@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
        PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters,
                           QObject *parent = nullptr);
        ~PowerManagementJob() override;

    protected:
        void start() override;

    private:
        QDBusPendingCall setScreenBrightness(int value, bool silent);
        QDBusPendingCall setKeyboardBrightness(int value, bool silent);
        SessionManagement *m_session;
};

#endif // POWERMANAGEMENTJOB_H
