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

#include <QObject>
#include <QDateTime>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "notificationmanager_export.h"

#include "notifications.h"

namespace NotificationManager
{

/**
 * @short Provides detailed information about a job
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT JobDetails : public QObject
{
    Q_OBJECT

    /**
     * User-friendly compact description text of the job,
     * for example "42 of 1337 files to "~/some/folder", or
     * "SomeFile.txt to Downloads".
     */
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

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

    Q_PROPERTY(qulonglong totalBytes READ totalBytes NOTIFY totalBytesChanged)
    Q_PROPERTY(qulonglong totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(qulonglong totalDirectories READ totalDirectories NOTIFY totalDirectoriesChanged)

    Q_PROPERTY(QString descriptionLabel1 READ descriptionLabel1 NOTIFY descriptionLabel1Changed)
    Q_PROPERTY(QString descriptionValue1 READ descriptionValue1 NOTIFY descriptionValue1Changed)

    Q_PROPERTY(QString descriptionLabel2 READ descriptionLabel2 NOTIFY descriptionLabel2Changed)
    Q_PROPERTY(QString descriptionValue2 READ descriptionValue2 NOTIFY descriptionValue2Changed)

    /**
     * This tries to generate a valid URL for a file that's being processed by converting descriptionValue2
     * (which is assumed to be the Destination) and if that isn't valid, descriptionValue1 to a URL.
     */
    Q_PROPERTY(QUrl descriptionUrl READ descriptionUrl NOTIFY descriptionUrlChanged)

public:
    explicit JobDetails(QObject *parent = nullptr);
    ~JobDetails() override;

    QString text() const;

    QUrl destUrl() const;

    qulonglong speed() const;

    qulonglong processedBytes() const;
    qulonglong processedFiles() const;
    qulonglong processedDirectories() const;

    qulonglong totalBytes() const;
    qulonglong totalFiles() const;
    qulonglong totalDirectories() const;

    QString descriptionLabel1() const;
    QString descriptionValue1() const;

    QString descriptionLabel2() const;
    QString descriptionValue2() const;

    QUrl descriptionUrl() const;

signals:
    void textChanged();
    void destUrlChanged();
    void speedChanged();
    void processedBytesChanged();
    void processedFilesChanged();
    void processedDirectoriesChanged();
    void processedAmountChanged();
    void totalBytesChanged();
    void totalFilesChanged();
    void totalDirectoriesChanged();
    void totalAmountChanged();
    void descriptionLabel1Changed();
    void descriptionValue1Changed();
    void descriptionLabel2Changed();
    void descriptionValue2Changed();
    void descriptionUrlChanged();

private:
    friend class Job;

    class Private;
    QScopedPointer<Private> d;

};

} // namespace NotificationManager
