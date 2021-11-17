/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDateTime>
#include <QString>
#include <QUrl>

#include "notifications.h"

#include "notificationmanager_export.h"

namespace NotificationManager
{
class JobPrivate;

/**
 * @short Represents a single job.
 *
 * A Job represents a special notification that has some progress information and in
 * some cases can be suspended or killed.
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 */
class NOTIFICATIONMANAGER_EXPORT Job : public QObject
{
    Q_OBJECT

    /**
     * The job infoMessage, e.g. "Copying".
     */
    Q_PROPERTY(QString summary READ summary NOTIFY summaryChanged)
    /**
     * User-friendly compact description text of the job,
     * for example "42 of 1337 files to "~/some/folder", or
     * "SomeFile.txt to Downloads".
     */
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

    /**
     * The desktop entry of the application owning the job, e.g. "org.kde.dolphin".
     */
    Q_PROPERTY(QString desktopEntry READ desktopEntry CONSTANT)
    /**
     * The user-visible name of the application owning the job, e.g. "Dolphin".
     */
    Q_PROPERTY(QString applicationName READ applicationName CONSTANT)
    /**
     * The icon name of the application owning the job, e.g. "system-file-manager".
     */
    Q_PROPERTY(QString applicationIconName READ applicationIconName CONSTANT)
    /**
     * The state the job is currently in.
     */
    Q_PROPERTY(Notifications::JobState state READ state NOTIFY stateChanged)
    /**
     * The total percentage (0-100) of job completion.
     */
    Q_PROPERTY(int percentage READ percentage NOTIFY percentageChanged)
    /**
     * The error code of the job failure.
     */
    Q_PROPERTY(int error READ error NOTIFY errorChanged)
    /**
     * Whether the job can be suspended.
     *
     * @sa Notifications::suspendJob
     * @sa Notifications::resumeJob
     */
    Q_PROPERTY(bool suspendable READ suspendable CONSTANT)
    /**
     * Whether the job can be aborted.
     *
     * @sa Notifications::killJob
     */
    Q_PROPERTY(bool killable READ killable CONSTANT)

    /**
     * The destination URL of a job.
     */
    Q_PROPERTY(QUrl destUrl READ destUrl NOTIFY destUrlChanged)

    /**
     * Current transfer rate in Byte/s
     */
    Q_PROPERTY(qulonglong speed READ speed NOTIFY speedChanged)

    Q_PROPERTY(qulonglong processedBytes READ processedBytes NOTIFY processedBytesChanged)
    Q_PROPERTY(qulonglong processedFiles READ processedFiles NOTIFY processedFilesChanged)
    Q_PROPERTY(qulonglong processedDirectories READ processedDirectories NOTIFY processedDirectoriesChanged)
    Q_PROPERTY(qulonglong processedItems READ processedItems NOTIFY processedItemsChanged)

    Q_PROPERTY(qulonglong totalBytes READ totalBytes NOTIFY totalBytesChanged)
    Q_PROPERTY(qulonglong totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(qulonglong totalDirectories READ totalDirectories NOTIFY totalDirectoriesChanged)
    Q_PROPERTY(qulonglong totalItems READ totalItems NOTIFY totalItemsChanged)

    Q_PROPERTY(QString descriptionLabel1 READ descriptionLabel1 NOTIFY descriptionLabel1Changed)
    Q_PROPERTY(QString descriptionValue1 READ descriptionValue1 NOTIFY descriptionValue1Changed)

    Q_PROPERTY(QString descriptionLabel2 READ descriptionLabel2 NOTIFY descriptionLabel2Changed)
    Q_PROPERTY(QString descriptionValue2 READ descriptionValue2 NOTIFY descriptionValue2Changed)

    /**
     * Whether there are any details available for this job.
     *
     * This is true as soon as any of the following are available:
     * - processed amount (of any unit)
     * - total amount (of any unit)
     * - description label or value (of any row)
     * - speed
     */
    Q_PROPERTY(bool hasDetails READ hasDetails NOTIFY hasDetailsChanged)

    /**
     * This tries to generate a valid URL for a file that's being processed by converting descriptionValue2
     * (which is assumed to be the Destination) and if that isn't valid, descriptionValue1 to a URL.
     */
    Q_PROPERTY(QUrl descriptionUrl READ descriptionUrl NOTIFY descriptionUrlChanged)

public:
    explicit Job(uint id, QObject *parent = nullptr);
    ~Job() override;

    uint id() const;

    QDateTime created() const;

    QDateTime updated() const;
    void resetUpdated();

    QString summary() const;
    QString text() const;

    QString desktopEntry() const;
    // TODO remove and let only constructor do it?
    void setDesktopEntry(const QString &desktopEntry);

    QString applicationName() const;
    // TODO remove and let only constructor do it?
    void setApplicationName(const QString &applicationName);

    QString applicationIconName() const;
    // TODO remove and let only constructor do it?
    void setApplicationIconName(const QString &applicationIconName);

    Notifications::JobState state() const;
    void setState(Notifications::JobState jobState);

    int percentage() const;

    int error() const;
    void setError(int error);

    QString errorText() const;
    void setErrorText(const QString &errorText);

    bool suspendable() const;
    // TODO remove and let only constructor do it?
    void setSuspendable(bool suspendable);

    bool killable() const;
    // TODO remove and let only constructor do it?
    void setKillable(bool killable);

    bool transient() const;
    void setTransient(bool transient);

    QUrl destUrl() const;

    qulonglong speed() const;

    qulonglong processedBytes() const;
    qulonglong processedFiles() const;
    qulonglong processedDirectories() const;
    qulonglong processedItems() const;

    qulonglong totalBytes() const;
    qulonglong totalFiles() const;
    qulonglong totalDirectories() const;
    qulonglong totalItems() const;

    QString descriptionLabel1() const;
    QString descriptionValue1() const;

    QString descriptionLabel2() const;
    QString descriptionValue2() const;

    bool hasDetails() const;

    QUrl descriptionUrl() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    Q_INVOKABLE void suspend();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void kill();

Q_SIGNALS:
    void updatedChanged();
    void summaryChanged();
    void textChanged();
    void stateChanged(Notifications::JobState jobState);
    void percentageChanged(int percentage);
    void errorChanged(int error);
    void errorTextChanged(const QString &errorText);
    void destUrlChanged();
    void speedChanged();
    void processedBytesChanged();
    void processedFilesChanged();
    void processedDirectoriesChanged();
    void processedItemsChanged();
    void processedAmountChanged();
    void totalBytesChanged();
    void totalFilesChanged();
    void totalDirectoriesChanged();
    void totalItemsChanged();
    void totalAmountChanged();
    void descriptionLabel1Changed();
    void descriptionValue1Changed();
    void descriptionLabel2Changed();
    void descriptionValue2Changed();
    void descriptionUrlChanged();
    void hasDetailsChanged();
    void expiredChanged();
    void dismissedChanged();

private:
    JobPrivate *d;

    friend class JobsModel;
    friend class JobsModelPrivate;
};

} // namespace NotificationManager
