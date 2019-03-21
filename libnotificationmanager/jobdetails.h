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
#include <QString>
#include <QStringList>
#include <QUrl>

#include "notificationmanager_export.h"

#include "notifications.h"

namespace NotificationManager
{

/**
 * @short Provides detail information about a job
 *
 * TODO
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

    Q_PROPERTY(QUrl destUrl MEMBER m_destUrl NOTIFY destUrlChanged)

    /**
     * Current transfer rate in Byte/s
     */
    Q_PROPERTY(qulonglong speed MEMBER m_speed NOTIFY speedChanged)

    // TODO eta?

    Q_PROPERTY(qulonglong processedBytes MEMBER m_processedBytes NOTIFY processedBytesChanged)
    Q_PROPERTY(qulonglong processedFiles MEMBER m_processedFiles NOTIFY processedFilesChanged)
    Q_PROPERTY(qulonglong processedDirectories MEMBER m_processedDirectories NOTIFY processedDirectoriesChanged)
    //Q_PROPERTY(qulonglong processedAmount MEMBER m_processedAmount NOTIFY processedAmountChanged)

    Q_PROPERTY(qulonglong totalBytes MEMBER m_totalBytes NOTIFY totalBytesChanged)
    Q_PROPERTY(qulonglong totalFiles MEMBER m_totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(qulonglong totalDirectories MEMBER m_totalDirectories NOTIFY totalDirectoriesChanged)
    //Q_PROPERTY(qulonglong totalAmount MEMBER m_totalAmount NOTIFY totalAmountChanged)

    Q_PROPERTY(QString descriptionLabel1 MEMBER m_descriptionLabel1 NOTIFY descriptionLabel1Changed)
    Q_PROPERTY(QString descriptionValue1 MEMBER m_descriptionValue1 NOTIFY descriptionValue1Changed)

    Q_PROPERTY(QString descriptionLabel2 MEMBER m_descriptionLabel2 NOTIFY descriptionLabel2Changed)
    Q_PROPERTY(QString descriptionValue2 MEMBER m_descriptionValue2 NOTIFY descriptionValue2Changed)

    /**
     * This tries to generate a valid URL for a file that's being processed by converting descriptionValue2
     * (which is assumed to be the Destination) and if that isn't valid, descriptionValue1 to a URL.
     */
    Q_PROPERTY(QUrl descriptionUrl READ descriptionUrl NOTIFY descriptionUrlChanged)

public:
    explicit JobDetails(QObject *parent = nullptr);
    ~JobDetails() override;

    QString text() const;
    QUrl descriptionUrl() const;

    void processData(const QVariantMap /*Plasma::DataEngine::Data*/ &data);

    friend class Job;

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
    QUrl m_destUrl;

    // FIXME d-pointer
    qulonglong m_speed = 0;

    qulonglong m_processedBytes = 0;
    qulonglong m_processedFiles = 0;
    qulonglong m_processedDirectories = 0;
    //qulonglong m_processedAmount = 0;

    qulonglong m_totalBytes = 0;
    qulonglong m_totalFiles = 0;
    qulonglong m_totalDirectories = 0;
    //qulonglong m_totalAmount = 0;

    QString m_descriptionLabel1;
    QString m_descriptionValue1;

    QString m_descriptionLabel2;
    QString m_descriptionValue2;

};

} // namespace NotificationManager


//Q_DECLARE_METATYPE(NotificationManager::JobDetails)
