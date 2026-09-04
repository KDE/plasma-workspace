/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "job.h"
#include "job_p.h"

#include <QDebug>
#include <QVariant>

using namespace NotificationManager;

Job::Job(uint id, QObject *parent)
    : QObject(parent)
    , d(new JobPrivate(id, this))
{
    d->m_created = QDateTime::currentDateTimeUtc();

    // These properties are used in generating the pretty job text
    connect(d, &JobPrivate::infoMessageChanged, this, &Job::textChanged);
    connect(this, &Job::processedFilesChanged, this, &Job::textChanged);
    connect(this, &Job::processedItemsChanged, this, &Job::textChanged);
    connect(this, &Job::totalFilesChanged, this, &Job::textChanged);
    connect(this, &Job::totalItemsChanged, this, &Job::textChanged);
    connect(this, &Job::descriptionValue1Changed, this, &Job::textChanged);
    connect(this, &Job::descriptionValue2Changed, this, &Job::textChanged);
    connect(this, &Job::destUrlChanged, this, &Job::textChanged);
    connect(this, &Job::errorTextChanged, this, &Job::textChanged);
    connect(this, &Job::destUrlChanged, this, &Job::effectiveDestUrlChanged);
    connect(this, &Job::descriptionUrlChanged, this, &Job::effectiveDestUrlChanged);
    connect(this, &Job::stateChanged, this, &Job::effectiveDestUrlChanged);
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
    Q_EMIT updatedChanged();
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
        Q_EMIT stateChanged(state);
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
        Q_EMIT errorChanged(error);
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
        Q_EMIT errorTextChanged(errorText);
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

bool Job::transient() const
{
    return d->m_transient;
}

void Job::setTransient(bool transient)
{
    d->m_transient = transient;
}

QUrl Job::destUrl() const
{
    return d->m_destUrl;
}

QUrl Job::effectiveDestUrl() const
{
    if (d->m_state != Notifications::JobState::JobStateStopped || d->m_error != 0) {
        return {};
    }

    QUrl url;
    // For a single file show the file url
    // Otherwise the destination folder
    if (d->m_totalFiles == 1) {
        url = d->descriptionUrl();
    } else {
        url = d->m_destUrl;
    }

    // Don't offer opening files in Trash
    if (url.scheme() == u"trash") {
        return {};
    }

    return url;
}

qulonglong Job::speed() const
{
    return d->m_speed;
}

QVariantList Job::speedHistory() const
{
    // A hundred points stand for the whole of the job, so a reading lands at the point its bytes
    // reach rather than at the moment it arrived. Each reading says how far the job had got and
    // when, so the speed over the stretch between two of them is the bytes they are apart over the
    // time they are apart, and the stretch is filled at that speed throughout. Working
    // both out here rather than as the readings arrive means a total that changes partway through,
    // which happens once a copy has finished working out what it is about to copy, simply places
    // the same readings elsewhere.
    constexpr int pointCount = 100;

    if (d->m_totalBytes == 0 || d->m_progressSamples.count() < 2) {
        return {};
    }

    const qreal bytesPerPoint = qreal(d->m_totalBytes) / pointCount;

    QVariantList history;
    history.reserve(pointCount);

    // A job can be further along than it said it would be: KIO raises the total only once it
    // notices, and until then everything beyond it belongs at the far end of the chart. Deciding
    // that in floating point keeps a job that claims a total of a few bytes from asking for a
    // point somewhere past the end of an int.
    const auto pointFor = [bytesPerPoint](qulonglong processedBytes) {
        const qreal position = processedBytes / bytesPerPoint;
        return position >= pointCount ? pointCount : qRound(position);
    };

    // The readings a stretch is worked out from are the last one drawn and the one which reaches the
    // next point, not simply neighbours: a job reports far more often than there are points to
    // draw, and taking whichever interval happened to cross the boundary would paint a whole
    // hundredth of the job at a moment's lull.
    auto drawnFrom = d->m_progressSamples.constFirst();

    for (int i = 1; i < d->m_progressSamples.count(); ++i) {
        const auto &current = d->m_progressSamples.at(i);

        const qint64 milliseconds = current.elapsedMilliseconds - drawnFrom.elapsedMilliseconds;
        // Signed: a job is free to report fewer bytes than it did before, and unsigned arithmetic
        // would turn that into a speed of some eighteen quintillion bytes a second.
        const qlonglong bytes = qlonglong(current.processedBytes) - qlonglong(drawnFrom.processedBytes);
        if (bytes < 0) {
            // It went backwards, so there is nothing to draw from where it was. Carry on from here.
            drawnFrom = current;
            continue;
        }

        const int point = pointFor(current.processedBytes);
        if (milliseconds <= 0 || point - history.count() <= 0) {
            continue;
        }

        const qreal speed = qreal(bytes) * 1000 / milliseconds;

        // Whatever the job did before the first reading was taken went unwatched, and drawing it
        // as a climb out of nothing would show an acceleration that never happened. What it managed
        // over that stretch is how far it had got by then over how long it had been going, so the
        // stretch is drawn at that, level. Whatever the reading itself caught, a burst or a lull,
        // says nothing about the time before it.
        if (history.isEmpty()) {
            // How long the job has been going, which is its own account of itself: the readings are
            // timed from when this job was first heard of, which for a job handed over to a service
            // that has just started is long after it began. A job which does not say has no time
            // behind it to average over, and then the reading is all there is.
            const qint64 elapsed = d->m_elapsedTime;
            const qreal averageUntilThen = elapsed > 0 ? qreal(drawnFrom.processedBytes) * 1000 / elapsed : speed;
            // Never past the point this reading reached, or the fill below would be asked for a
            // negative number of points.
            history.insert(history.end(), qMin(pointFor(drawnFrom.processedBytes), point), averageUntilThen);
        }

        history.insert(history.end(), point - history.count(), speed);
        drawnFrom = current;
    }

    return history;
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

qint64 Job::elapsedTime() const
{
    return d->m_elapsedTime;
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
        Q_EMIT expiredChanged();
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
        Q_EMIT dismissedChanged();
    }
}

void Job::suspend()
{
    Q_EMIT d->suspendRequested();
}

void Job::resume()
{
    Q_EMIT d->resumeRequested();
}

void Job::kill()
{
    d->kill();
}

#include "moc_job.cpp"
