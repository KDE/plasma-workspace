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

#include <KFilePlacesModel>
#include <KLocalizedString>

#include "jobdetails_p.h"

using namespace NotificationManager;

JobDetails::Private::Private(JobDetails *q)
    : q(q)
    , m_placesModel(createPlacesModel())
{

}

JobDetails::Private::~Private() = default;

QSharedPointer<KFilePlacesModel> JobDetails::Private::createPlacesModel()
{
    static QWeakPointer<KFilePlacesModel> s_instance;
    if (!s_instance) {
        QSharedPointer<KFilePlacesModel> ptr(new KFilePlacesModel());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

// Tries to return a more user-friendly displayed destination
// - if it is a place, show the name, e.g. "Downloads"
// - if it is inside home, abbreviate that to tilde ~/foo
// - otherwise print URL (without password)
QString JobDetails::Private::prettyDestUrl() const
{
    if (!m_destUrl.isValid()) {
        return QString();
    }

    if (!m_placesModel) {
        m_placesModel = createPlacesModel();
    }

    // If we copy into a "place", show its pretty name instead of a URL/path
    for (int row = 0; row < m_placesModel->rowCount(); ++row) {
        const QModelIndex idx = m_placesModel->index(row, 0);
        if (m_placesModel->isHidden(idx)) {
            continue;
        }

        if (m_placesModel->url(idx).matches(m_destUrl, QUrl::StripTrailingSlash)) {
            return m_placesModel->text(idx);
        }
    }

    if (m_destUrl.isLocalFile()) {
        QString destUrlString = m_destUrl.toLocalFile();

        const QString homePath = QDir::homePath();
        if (destUrlString.startsWith(homePath)) {
            destUrlString = QStringLiteral("~") + destUrlString.mid(homePath.length());
        }

        return destUrlString;
    }

    return m_destUrl.toDisplayString(); // strips password
}

QString JobDetails::Private::text() const
{
    const QString currentFileName = descriptionUrl().fileName();
    const QString destUrlString = prettyDestUrl();

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


void JobDetails::Private::processData(const QVariantMap &data)
{
    bool textDirty = false;
    bool urlDirty = false;

    auto it = data.find(QStringLiteral("destUrl"));
    if (it != data.end()) {
        const QUrl destUrl = it->toUrl();
        if (m_destUrl != destUrl) {
            m_destUrl = destUrl;
            textDirty = true;
            emit q->destUrlChanged();
        }
    }

    processField(data, QStringLiteral("labelName0"), m_descriptionLabel1,
                 q, &JobDetails::descriptionLabel1Changed);
    if (processField(data, QStringLiteral("label0"), m_descriptionValue1,
                     q, &JobDetails::descriptionValue1Changed)) {
        textDirty = true;
        urlDirty = true;
    }

    processField(data, QStringLiteral("labelName1"), m_descriptionLabel2,
                 q, &JobDetails::descriptionLabel2Changed);
    if (processField(data, QStringLiteral("label1"), m_descriptionValue2,
                     q, &JobDetails::descriptionValue2Changed)) {
        textDirty = true;
        urlDirty = true;
    }

    processField(data, QStringLiteral("numericSpeed"), m_speed,
                 q, &JobDetails::speedChanged);

    for (int i = 0; i <= 2; ++i) {
        {
            const QString unit = data.value(QStringLiteral("processedUnit%1").arg(QString::number(i))).toString();
            const QString amountKey = QStringLiteral("processedAmount%1").arg(QString::number(i));

            if (unit == QLatin1String("bytes")){
                processField(data, amountKey, m_processedBytes, q, &JobDetails::processedBytesChanged);
            } else if (unit == QLatin1String("files")) {
                if (processField(data, amountKey, m_processedFiles, q, &JobDetails::processedFilesChanged)) {
                    textDirty = true;
                }
            } else if (unit == QLatin1String("dirs")) {
                processField(data, amountKey, m_processedDirectories, q, &JobDetails::processedDirectoriesChanged);
            }
        }

        {
            const QString unit = data.value(QStringLiteral("totalUnit%1").arg(QString::number(i))).toString();
            const QString amountKey = QStringLiteral("totalAmount%1").arg(QString::number(i));

            if (unit == QLatin1String("bytes")){
                processField(data, amountKey, m_totalBytes, q, &JobDetails::totalBytesChanged);
            } else if (unit == QLatin1String("files")) {
                if (processField(data, amountKey, m_totalFiles, q, &JobDetails::totalFilesChanged)) {
                    textDirty = true;
                }
            } else if (unit == QLatin1String("dirs")) {
                processField(data, amountKey, m_totalDirectories, q, &JobDetails::totalDirectoriesChanged);
            }
        }
    }

    if (urlDirty) {
        emit q->descriptionUrlChanged();
    }
    if (urlDirty || textDirty) {
        emit q->textChanged();
    }
}

QUrl JobDetails::Private::descriptionUrl() const
{
    QUrl url = QUrl::fromUserInput(m_descriptionValue2, QString(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        url = QUrl::fromUserInput(m_descriptionValue1, QString(), QUrl::AssumeLocalFile);
    }
    return url;
}

JobDetails::JobDetails(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{

}

JobDetails::~JobDetails() = default;

QString JobDetails::text() const
{
    return d->text();
}

QUrl JobDetails::destUrl() const
{
    return d->m_destUrl;
}

qulonglong JobDetails::speed() const
{
    return d->m_speed;
}

qulonglong JobDetails::processedBytes() const
{
    return d->m_processedBytes;
}

qulonglong JobDetails::processedFiles() const
{
    return d->m_processedFiles;
}

qulonglong JobDetails::processedDirectories() const
{
    return d->m_processedDirectories;
}

qulonglong JobDetails::totalBytes() const
{
    return d->m_totalBytes;
}

qulonglong JobDetails::totalFiles() const
{
    return d->m_totalFiles;
}

qulonglong JobDetails::totalDirectories() const
{
    return d->m_totalDirectories;
}

QString JobDetails::descriptionLabel1() const
{
    return d->m_descriptionLabel1;
}

QString JobDetails::descriptionValue1() const
{
    return d->m_descriptionValue1;
}

QString JobDetails::descriptionLabel2() const
{
    return d->m_descriptionLabel2;
}

QString JobDetails::descriptionValue2() const
{
    return d->m_descriptionValue2;
}

QUrl JobDetails::descriptionUrl() const
{
    return d->descriptionUrl();
}
