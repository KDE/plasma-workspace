/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "job.h"

#include <QDebug>

#include <KService>

#include "notifications.h"

#include "jobdetails.h"
#include "jobdetails_p.h"

#include <QQmlEngine>

using namespace NotificationManager;

Job::Job(const QString &sourceName)
    : m_sourceName(sourceName)
    , m_created(QDateTime::currentDateTimeUtc())
    , m_details(new JobDetails())
{
    // FIXME still needed?
    QQmlEngine::setObjectOwnership(m_details, QQmlEngine::CppOwnership);
}

Job::~Job()
{
    delete m_details;
}

QString Job::sourceName() const
{
    return m_sourceName;
}

QDateTime Job::created() const
{
    return m_created;
}

QDateTime Job::updated() const
{
    return m_updated;
}

void Job::setUpdated()
{
    m_updated = QDateTime::currentDateTimeUtc();
}

QString Job::summary() const
{
    return m_summary;
}

QString Job::applicationName() const
{
    return m_applicationName;
}

QString Job::applicationIconName() const
{
    return m_applicationIconName;
}

Notifications::JobState Job::state() const
{
    return m_state;
}

int Job::percentage() const
{
    return m_percentage;
}

int Job::error() const
{
    return m_error;
}

QString Job::errorText() const
{
    return m_errorText;
}

bool Job::suspendable() const
{
    return m_suspendable;
}

bool Job::killable() const
{
    return m_killable;
}

JobDetails *Job::details() const
{
    return m_details;
}

bool Job::expired() const
{
    return m_expired;
}

void Job::setExpired(bool expired)
{
    m_expired = expired;
}

bool Job::dismissed() const
{
    return m_dismissed;
}

void Job::setDismissed(bool dismissed)
{
    m_dismissed = dismissed;
}

template<typename T> void processField(const QVariantMap/*Plasma::DataEngine::Data*/ &data,
                                            const QString &field,
                                            T &target,
                                            int role,
                                            QVector<int> &dirtyRoles)
{
    auto it = data.find(field);
    if (it != data.end()) {
        const T newValue = it->value<T>();
        if (target != newValue) {
            target = newValue;
            dirtyRoles.append(role);
        }
    }
}

QVector<int> Job::processData(const QVariantMap/*Plasma::DataEngine::Data*/ &data)
{
    QVector<int> dirtyRoles;

    auto end = data.end();

    processField(data, QStringLiteral("infoMessage"), m_summary, Notifications::SummaryRole, dirtyRoles);
    processField(data, QStringLiteral("percentage"), m_percentage, Notifications::PercentageRole, dirtyRoles);
    processField(data, QStringLiteral("errorText"), m_errorText, Notifications::ErrorTextRole, dirtyRoles);
    processField(data, QStringLiteral("error"), m_error, Notifications::ErrorRole, dirtyRoles);

    /*if (m_errorText.isEmpty() && m_error) {
        m_errorText = KIO::buildErrorString(m_error);
        dirtyRoles.append(Notifications::ErrorTextRole);
    }*/

    processField(data, QStringLiteral("killable"), m_killable, Notifications::KillableRole, dirtyRoles);
    processField(data, QStringLiteral("suspendable"), m_suspendable, Notifications::SuspendableRole, dirtyRoles);

    auto it = data.find("appName");
    if (it != end) {
        const QString appName = it->toString();
        if (m_appName != appName) {
            m_appName = appName;

            QString applicationName = appName;
            QString applicationIconName = data.value(QStringLiteral("appIconName")).toString();

            KService::Ptr service = KService::serviceByStorageId(appName);
            if (!service) {
                // HACK would be nice to add a JobViewV3 which works on desktop entries instead of app names
                // cf. KUiServerJobTracker::registerJob in kjobwidgets
                service = KService::serviceByStorageId(QStringLiteral("org.kde.") + appName);
            }

            if (service) {
                applicationName = service->name();
                applicationIconName = service->icon();
            }

            if (m_applicationName != applicationName) {
                m_applicationName = applicationName;
                dirtyRoles.append(Notifications::ApplicationNameRole);
            }

            if (m_applicationIconName != applicationIconName) {
                m_applicationIconName = applicationIconName;
                dirtyRoles.append(Notifications::ApplicationIconNameRole);
            }
        }
    }

    it = data.find(QStringLiteral("state"));
    if (it != end) {
        const QString stateString = it->toString();

        Notifications::JobState state = Notifications::JobStateRunning;

        if (stateString == QLatin1String("suspended")) {
            state = Notifications::JobStateSuspended;
        } else if (stateString == QLatin1String("stopped")) {
            state = Notifications::JobStateStopped;
        }

        if (m_state != state) {
            m_state = state;
            dirtyRoles.append(Notifications::JobStateRole);
        }
    }

    m_details->d->processData(data);

    return dirtyRoles;
}

bool Job::operator==(const Job &other) const
{
    return other.m_sourceName == m_sourceName;
}
