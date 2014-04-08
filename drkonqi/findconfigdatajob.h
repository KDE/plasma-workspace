/*******************************************************************
* findconfigdatajob.h
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

#ifndef DRKONQI_FIND_VERSIONS_JOB_H
#define DRKONQI_FIND_VERSIONS_JOB_H

#include <QtCore/QStringList>

#include <KJob>
#include <QUrl>

namespace KIO {
    class StoredTransferJob;
}

/**
 * This job downloads config.cgi for a specified product,
 * looking for the information you want to retrieve
 */
class FindConfigDataJob : public KJob
{
    Q_OBJECT
    public:
        /**
         * @param productName e.g. "plasma"
         * @param bugtrackerBaseUrl e.g. "https://bugs.kde.org"
         */
        explicit FindConfigDataJob(const QString &productName, const QUrl &bugtrackerBaseUrl, QObject *parent = 0);
        virtual ~FindConfigDataJob();

        virtual void start();
        virtual QString errorString() const;

        enum InformationType {
            Version
        };

        /**
         * Call this after the job finished to retrieve the
         * specified data
         */
        QStringList data(InformationType type);

    private slots:
        void receivedData(KJob *job);

    private:
        KIO::StoredTransferJob *m_job;
        QUrl m_url;
        QString m_data;
        QString m_errorString;
};

#endif
