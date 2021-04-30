/*
    Copyright (C) 2021 Kai Uwe Broulik <kde@broulik.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "fileinfo.h"

#include <QMimeDatabase>
#include <QTimer>

#include <KApplicationTrader>
#include <KIO/MimeTypeFinderJob>

Application::Application() = default;

Application::Application(const KService::Ptr &service)
    : m_service(service)
{
}

QString Application::name() const
{
    return m_service ? m_service->name() : QString();
}

QString Application::iconName() const
{
    return m_service ? m_service->icon() : QString();
}

bool Application::isValid() const
{
    return m_service && m_service->isValid();
}

FileInfo::FileInfo(QObject *parent)
    : QObject(parent)
{
}

FileInfo::~FileInfo() = default;

QUrl FileInfo::url() const
{
    return m_url;
}

void FileInfo::setUrl(const QUrl &url)
{
    if (m_url != url) {
        m_url = url;
        reload();
        emit urlChanged(url);
    }
}

bool FileInfo::busy() const
{
    return m_busy;
}

void FileInfo::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        emit busyChanged(busy);
    }
}

int FileInfo::error() const
{
    return m_error;
}

void FileInfo::setError(int error)
{
    if (m_error != error) {
        m_error = error;
        emit errorChanged(error);
    }
}

QString FileInfo::mimeType() const
{
    return m_mimeType;
}

QString FileInfo::iconName() const
{
    return m_iconName;
}

Application FileInfo::preferredApplication() const
{
    return m_preferredApplication;
}

void FileInfo::reload()
{
    if (!m_url.isValid()) {
        return;
    }

    if (m_job) {
        m_job->kill();
    }

    setError(0);

    // Do a quick guess by file name while we wait for the job to find the mime type
    QString guessedMimeType;

    // NOTE using QUrl::path() for API that accepts local files is usually wrong
    // but here we really only care about the file name and its extension.
    const auto type = QMimeDatabase().mimeTypeForFile(m_url.path(), QMimeDatabase::MatchExtension);
    if (!type.isDefault()) {
        guessedMimeType = type.name();
    }

    mimeTypeFound(guessedMimeType);

    m_job = new KIO::MimeTypeFinderJob(m_url);
    m_job->setAuthenticationPromptEnabled(false);

    const QUrl url = m_url;
    connect(m_job, &KIO::MimeTypeFinderJob::result, this, [this, url] {
        setError(m_job->error());
        if (m_job->error()) {
            qWarning() << "Failed to determine mime type for" << url << m_job->errorString();
        } else {
            mimeTypeFound(m_job->mimeType());
        }
        setBusy(false);
    });

    setBusy(true);
    m_job->start();
}

void FileInfo::mimeTypeFound(const QString &mimeType)
{
    if (m_mimeType == mimeType) {
        return;
    }

    m_mimeType = mimeType;

    KService::Ptr preferredApp;

    if (!mimeType.isEmpty()) {
        const auto type = QMimeDatabase().mimeTypeForName(mimeType);
        m_iconName = type.iconName();

        preferredApp = KApplicationTrader::preferredService(mimeType);
    } else {
        m_iconName.clear();
    }

    m_preferredApplication = Application(preferredApp);

    emit mimeTypeChanged();
}
