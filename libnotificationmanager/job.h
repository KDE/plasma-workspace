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

#pragma once

#include <QDateTime>
#include <QString>
#include <QUrl>

//#include "notificationmanager_export.h"

#include "notifications.h"


namespace NotificationManager
{

class JobDetails;

/**
 * @short Represents a single notification
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class Job
{
public:
    explicit Job(const QString &sourceName = QString());
    ~Job();

    QString sourceName() const;

    QDateTime created() const;

    QDateTime updated() const;
    void setUpdated();

    QString summary() const;

    QString body() const;
    /*void setBody(const QString &body);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QImage image() const;
    void setImage(const QImage &image);*/

    QString applicationName() const;

    QString applicationIconName() const;

    Notifications::JobState state() const;
    int percentage() const;
    int error() const;
    QString errorText() const;
    bool suspendable() const;
    bool killable() const;

    JobDetails *details() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    QVector<int> processData(const QVariantMap/*Plasma::DataEngine::Data*/ &data);

    bool operator==(const Job &other) const;

    friend class JobsModel;

private:
    QString m_sourceName;

    QDateTime m_created;
    QDateTime m_updated;

    QString m_summary;

    // raw appName and appIconName from kuiserver
    QString m_appName;
    QString m_appIconName;
    // names looked up from a service
    QString m_applicationName;
    QString m_applicationIconName;

    Notifications::JobState m_state = Notifications::JobStateStopped;
    int m_percentage = 0;
    int m_error = 0;
    QString m_errorText;
    bool m_suspendable = false;
    bool m_killable = false;

    JobDetails *m_details = nullptr;

    bool m_expired = false;
    bool m_dismissed = false;

};

} // namespace NotificationManager
