/*******************************************************************
* findconfigdatajob.cpp
* Copyright 2011    Matthias Fuchs <mat69@gmx.net>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/

#include "findconfigdatajob.h"

#include <KIO/Job>
#include <KLocalizedString>

FindConfigDataJob::FindConfigDataJob(const QString &productName, const QUrl &bugtrackerBaseUrl, QObject *parent)
  : KJob(parent),
    m_job(0),
    m_url(bugtrackerBaseUrl)
{
    m_url.addPath("config.cgi");
    m_url.addQueryItem("product", productName);
}

FindConfigDataJob::~FindConfigDataJob()
{
    if (m_job) {
        m_job->kill();
    }
}

void FindConfigDataJob::start()
{
    m_job = KIO::storedGet(m_url, KIO::Reload, KIO::HideProgressInfo);
    connect(m_job, &KIO::StoredTransferJob::result, this, &FindConfigDataJob::receivedData);
    connect(m_job, &KIO::StoredTransferJob::infoMessage, this, &FindConfigDataJob::infoMessage);
    connect(m_job, &KIO::StoredTransferJob::warning, this, &FindConfigDataJob::warning);

    m_job->start();
}

QString FindConfigDataJob::errorString() const
{
    return m_errorString;
}

void FindConfigDataJob::receivedData(KJob *job)
{
    Q_UNUSED(job);

    if (m_job->error()) {
        setError(m_job->error());
        m_errorString = i18n("Failed to retrieve the config data.");
    } else {
        m_data = m_job->data();
    }
    m_job = 0;
    emitResult();
}

QStringList FindConfigDataJob::data(InformationType type)
{
    QStringList result;
    QString key;

    switch (type) {
    case Version:
        key = "version\\['[^\']+'\\]";
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    QRegExp rx(key + " = \\[ (('[^\']+', )+) \\];");
    if (rx.indexIn(m_data) != -1) {
        QString temp = rx.cap(1);
        temp.remove('\'');
        temp.remove(' ');
        result = temp.split(',', QString::SkipEmptyParts);
    }

    kDebug() << "Found data for " + key + ':' << result << result.count();
    return result;
}


