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

#include "jobdetails.h"

#include <QDir>
#include <QDebug>

#include <KLocalizedString>

using namespace NotificationManager;

JobDetails::JobDetails(QObject *parent) : QObject(parent)
{

}

JobDetails::~JobDetails() = default;

QString JobDetails::text() const
{
    const QString currentFileName = descriptionUrl().fileName();

    QString destUrlString;
    if (m_destUrl.isLocalFile()) {
        destUrlString = m_destUrl.toLocalFile();

        const QString homePath = QDir::homePath();
        if (destUrlString.startsWith(homePath)) {
            destUrlString = QStringLiteral("~") + destUrlString.mid(homePath.length());
        }
    } else {
        destUrlString = m_destUrl.toDisplayString(); // strips password
    }

    qDebug() << "JOB DETAILS" << "current file name" << currentFileName << "desturl" << destUrlString
               << "processed files" << m_processedFiles << "total files" << m_totalFiles
               << "processed dirs" << m_processedDirectories << "total dirs" << m_totalDirectories
               << "dest url" << m_destUrl << "label1" << m_descriptionLabel1 << "value1" << m_descriptionValue1
               << "label2" << m_descriptionLabel2 << "value2" << m_descriptionValue2;

    if (m_totalFiles == 0) {
        if (!destUrlString.isEmpty()) {
            return i18nc("Copying unknown amount of files to location", "to %1", destUrlString);
        }
    } else if (m_totalFiles == 1 && !currentFileName.isEmpty()) {
        if (!destUrlString.isEmpty()) {
            return i18nc("Copying file to location", "%1 to %2", currentFileName, destUrlString);
        }

        return currentFileName;
    } else if (m_totalFiles > 1) {
        if (!destUrlString.isEmpty()) {
            if (m_processedFiles > 0) {
                return i18ncp("Copying n of m files to locaton", "%2 of %1 file to %3", "%2 of %1 files to %3",
                              m_totalFiles, m_processedFiles, destUrlString);
            }
            return i18ncp("Copying n files to location", "%1 file to %2", "%1 files to %2",
                          m_totalFiles, destUrlString);
        }

        if (m_processedFiles > 0) {
            return i18ncp("Copying n of m files", "%2 of %1 file", "%2 of %1 files",
                          m_totalFiles, m_processedFiles);
        }

        return i18ncp("Copying n files", "%1 file", "%1 files", m_totalFiles);
    }

    qWarning() << "  NO DETAILS";
    return QString();
}

QUrl JobDetails::descriptionUrl() const
{
    QUrl url = QUrl::fromUserInput(m_descriptionValue2, QString(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        url = QUrl::fromUserInput(m_descriptionValue1, QString(), QUrl::AssumeLocalFile);
    }
    return url;
}

template<typename T> bool processField(const QVariantMap/*Plasma::DataEngine::Data*/ &data,
                                       const QString &field,
                                       T &target,
                                       JobDetails *details,
                                       void (JobDetails::*changeSignal)())
{
    auto it = data.find(field);
    if (it != data.end()) {
        const T newValue = it->value<T>();
        if (target != newValue) {
            target = newValue;
            emit ((details)->*changeSignal)();
            return true;
        }
    }
    return false;
}


void JobDetails::processData(const QVariantMap &data)
{
    bool textDirty = false;
    bool urlDirty = false;

    auto it = data.find(QStringLiteral("destUrl"));
    if (it != data.end()) {
        const QUrl destUrl = it->toUrl();
        if (m_destUrl != destUrl) {
            m_destUrl = destUrl;
            textDirty = true;
            emit destUrlChanged();
        }
    }

    processField(data, QStringLiteral("labelName0"), m_descriptionLabel1,
                 this, &JobDetails::descriptionLabel1Changed);
    if (processField(data, QStringLiteral("label0"), m_descriptionValue1,
                     this, &JobDetails::descriptionValue1Changed)) {
        textDirty = true;
        urlDirty = true;
    }

    processField(data, QStringLiteral("labelName1"), m_descriptionLabel2,
                 this, &JobDetails::descriptionLabel2Changed);
    if (processField(data, QStringLiteral("label1"), m_descriptionValue2,
                     this, &JobDetails::descriptionValue2Changed)) {
        textDirty = true;
        urlDirty = true;
    }

    processField(data, QStringLiteral("numericSpeed"), m_speed,
                 this, &JobDetails::speedChanged);

    for (int i = 0; i <= 2; ++i) {
        {
            const QString unit = data.value(QStringLiteral("processedUnit%1").arg(QString::number(i))).toString();
            const QString amountKey = QStringLiteral("processedAmount%1").arg(QString::number(i));

            if (unit == QLatin1String("bytes")){
                processField(data, amountKey, m_processedBytes, this, &JobDetails::processedBytesChanged);
            } else if (unit == QLatin1String("files")) {
                if (processField(data, amountKey, m_processedFiles, this, &JobDetails::processedFilesChanged)) {
                    textDirty = true;
                }
            } else if (unit == QLatin1String("dirs")) {
                processField(data, amountKey, m_processedDirectories, this, &JobDetails::processedDirectoriesChanged);
            }
        }

        {
            const QString unit = data.value(QStringLiteral("totalUnit%1").arg(QString::number(i))).toString();
            const QString amountKey = QStringLiteral("totalAmount%1").arg(QString::number(i));

            if (unit == QLatin1String("bytes")){
                processField(data, amountKey, m_totalBytes, this, &JobDetails::totalBytesChanged);
            } else if (unit == QLatin1String("files")) {
                if (processField(data, amountKey, m_totalFiles, this, &JobDetails::totalFilesChanged)) {
                    textDirty = true;
                }
            } else if (unit == QLatin1String("dirs")) {
                processField(data, amountKey, m_totalDirectories, this, &JobDetails::totalDirectoriesChanged);
            }
        }
    }

    if (urlDirty) {
        emit descriptionUrlChanged();
    }
    if (urlDirty || textDirty) {
        emit textChanged();
    }
}
