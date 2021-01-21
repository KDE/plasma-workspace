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
#include "job_p.h"

#include <QDebug>

#include <KService>

using namespace NotificationManager;

Job::Job(uint id, QObject *parent)
    : QObject(parent)
    , d(new JobPrivate(id, this))
{
    d->m_created = QDateTime::currentDateTimeUtc();

    // These properties are used in generating the pretty job text
    connect(d, &JobPrivate::infoMessageChanged, this, &Job::textChanged);
    connect(this, &Job::processedFilesChanged, this, &Job::textChanged);
    connect(this, &Job::totalFilesChanged, this, &Job::textChanged);
    connect(this, &Job::descriptionValue1Changed, this, &Job::textChanged);
    connect(this, &Job::descriptionValue2Changed, this, &Job::textChanged);
    connect(this, &Job::destUrlChanged, this, &Job::textChanged);
    connect(this, &Job::errorTextChanged, this, &Job::textChanged);
}

Job::~Job() = default;

uint Job::id() const
{
    return d->m_id;
}

QDateTime Job::created() const
{
    return d->m_created;
}

QDateTime Job::updated() const
{
    return d->m_updated;
}

void Job::resetUpdated()
{
    d->m_updated = QDateTime::currentDateTimeUtc();
    emit updatedChanged();
}

QString Job::summary() const
{
    return d->m_summary;
}

QString Job::text() const
{
    return d->text();
}

QString Job::desktopEntry() const
{
    return d->m_desktopEntry;
}

void Job::setDesktopEntry(const QString &desktopEntry)
{
    Q_ASSERT(d->m_desktopEntry.isNull());
    d->m_desktopEntry = desktopEntry;
}

QString Job::applicationName() const
{
    return d->m_applicationName;
}

void Job::setApplicationName(const QString &applicationName)
{
    Q_ASSERT(d->m_applicationName.isNull());
    d->m_applicationName = applicationName;
}

QString Job::applicationIconName() const
{
    return d->m_applicationIconName;
}

void Job::setApplicationIconName(const QString &applicationIconName)
{
    Q_ASSERT(d->m_applicationIconName.isNull());
    d->m_applicationIconName = applicationIconName;
}

Notifications::JobState Job::state() const
{
    return d->m_state;
}

void Job::setState(Notifications::JobState state)
{
    if (d->m_state != state) {
        d->m_state = state;
        emit stateChanged(state);
    }
}

int Job::percentage() const
{
    return d->m_percentage;
}

int Job::error() const
{
    return d->m_error;
}

void Job::setError(int error)
{
    if (d->m_error != error) {
        d->m_error = error;
        emit errorChanged(error);
    }
}

QString Job::errorText() const
{
    return d->m_errorText;
}

void Job::setErrorText(const QString &errorText)
{
    if (d->m_errorText != errorText) {
        d->m_errorText = errorText;
        emit errorTextChanged(errorText);
    }
}

bool Job::suspendable() const
{
    return d->m_suspendable;
}

void Job::setSuspendable(bool suspendable)
{
    // Cannot change after job started
    d->m_suspendable = suspendable;
}

bool Job::killable() const
{
    return d->m_killable;
}

void Job::setKillable(bool killable)
{
    // Cannot change after job started
    d->m_killable = killable;
}

QUrl Job::destUrl() const
{
    return d->m_destUrl;
}

qulonglong Job::speed() const
{
    return d->m_speed;
}

qulonglong Job::processedBytes() const
{
    return d->m_processedBytes;
}

qulonglong Job::processedFiles() const
{
    return d->m_processedFiles;
}

qulonglong Job::processedDirectories() const
{
    return d->m_processedDirectories;
}

qulonglong Job::processedItems() const
{
    return d->m_processedItems;
}

qulonglong Job::totalBytes() const
{
    return d->m_totalBytes;
}

qulonglong Job::totalFiles() const
{
    return d->m_totalFiles;
}

qulonglong Job::totalDirectories() const
{
    return d->m_totalDirectories;
}

qulonglong Job::totalItems() const
{
    return d->m_totalItems;
}

QString Job::descriptionLabel1() const
{
    return d->m_descriptionLabel1;
}

QString Job::descriptionValue1() const
{
    return d->m_descriptionValue1;
}

QString Job::descriptionLabel2() const
{
    return d->m_descriptionLabel2;
}

QString Job::descriptionValue2() const
{
    return d->m_descriptionValue2;
}

bool Job::hasDetails() const
{
    return d->m_hasDetails;
}

QUrl Job::descriptionUrl() const
{
    return d->descriptionUrl();
}

bool Job::expired() const
{
    return d->m_expired;
}

void Job::setExpired(bool expired)
{
    if (d->m_expired != expired) {
        d->m_expired = expired;
        emit expiredChanged();
    }
}

bool Job::dismissed() const
{
    return d->m_dismissed;
}

void Job::setDismissed(bool dismissed)
{
    if (d->m_dismissed != dismissed) {
        d->m_dismissed = dismissed;
        emit dismissedChanged();
    }
}

void Job::suspend()
{
    emit d->suspendRequested();
}

void Job::resume()
{
    emit d->resumeRequested();
}

void Job::kill()
{
    d->kill();
}
