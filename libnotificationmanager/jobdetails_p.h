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
#include <QStringList>
#include <QUrl>

#include "jobdetails.h"

class KFilePlacesModel;

namespace NotificationManager
{

class JobDetails;

class JobDetails::Private
{
public:
    Private(JobDetails *q);
    ~Private();

    QString prettyDestUrl() const;
    QString text() const;
    QUrl descriptionUrl() const;

    void processData(const QVariantMap /*Plasma::DataEngine::Data*/ &data);

public:
    QUrl m_destUrl;

    qulonglong m_speed = 0;

    qulonglong m_processedBytes = 0;
    qulonglong m_processedFiles = 0;
    qulonglong m_processedDirectories = 0;

    qulonglong m_totalBytes = 0;
    qulonglong m_totalFiles = 0;
    qulonglong m_totalDirectories = 0;

    QString m_descriptionLabel1;
    QString m_descriptionValue1;

    QString m_descriptionLabel2;
    QString m_descriptionValue2;

private:
    static QSharedPointer<KFilePlacesModel> createPlacesModel();

    JobDetails *q;

    mutable QSharedPointer<KFilePlacesModel> m_placesModel;

};

} // namespace NotificationManager
