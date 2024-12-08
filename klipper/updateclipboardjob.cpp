/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "updateclipboardjob.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QImage>
#include <QImageWriter>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

#include <KIO/MkdirJob>
#include <KIO/StoredTransferJob>

using namespace Qt::StringLiterals;

UpdateDatabaseJob *UpdateDatabaseJob::updateClipboard(QObject *parent,
                                                      QSqlDatabase *database,
                                                      QStringView databaseFolder,
                                                      const QString &uuid,
                                                      const QString &text,
                                                      const QMimeData *mimeData,
                                                      qreal timestamp)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    bool hasImage = false;
    std::list<MimeData> mimeDataList;
    const QStringList formats = mimeData->formats();
    for (const QString &format : formats) {
        if (!format.contains(u'/')) {
            continue;
        }
        hash.reset();
        QByteArray data;
        if (format.startsWith(u"image/") || format == u"application/x-qt-image") {
            if (!hasImage) {
                hasImage = true;
                QImage image = mimeData->imageData().value<QImage>();
                hash.addData(QByteArrayView(reinterpret_cast<const char *>(image.constBits()), image.sizeInBytes()));
                QBuffer buffer(&data);
                QImageWriter encoder(&buffer, "PNG");
                encoder.write(image);
                mimeDataList.emplace_back(s_imageFormat, std::move(data), QString::fromLatin1(hash.result().toHex()));
            }
        } else {
            data = mimeData->data(format);
            if (data.size() > 20 * 1000 * 1000) {
                // Skip anything greater than 20MB because we don't want too
                // many heavy things to be persistently held in the clipboard.
                continue;
            }
            hash.addData(data);
            mimeDataList.emplace_back(format, std::move(data), QString::fromLatin1(hash.result().toHex()));
        }
    }

    return new UpdateDatabaseJob(parent, database, databaseFolder, uuid, text, formats, std::move(mimeDataList), timestamp);
}

UpdateDatabaseJob::UpdateDatabaseJob(QObject *parent,
                                     QSqlDatabase *database,
                                     QStringView databaseFolder,
                                     const QString &uuid,
                                     const QString &text,
                                     const QStringList &formats,
                                     std::list<MimeData> &&mimeDataList,
                                     qreal timestamp)
    : KCompositeJob(parent)
    , m_db(database)
    , m_uuid(uuid)
    , m_text(text)
    , m_formats(formats)
    , m_dataDir(databaseFolder + u"/data/")
    , m_mimeDataList(std::move(mimeDataList))
    , m_timestamp(timestamp)
{
}

UpdateDatabaseJob::~UpdateDatabaseJob()
{
}

void UpdateDatabaseJob::start()
{
    if (!m_db->isOpen()) [[unlikely]] {
        setErrorText(u"Invalid database"_s);
        setError(DatabaseError);
        emitResult();
        return;
    }

    QSqlQuery query(*m_db);
    query.prepare(u"INSERT INTO main (uuid, added_time, last_used_time, mimetypes, text) VALUES (?, ?, ?, ?, ?)"_s);
    query.addBindValue(m_uuid);
    if (m_timestamp == 0) [[likely]] {
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
    } else {
        query.addBindValue(qreal(m_timestamp));
        query.addBindValue(qreal(m_timestamp));
    }
    query.addBindValue(m_formats.join(u','));
    query.addBindValue(m_text);
    if (!query.exec()) {
        setErrorText(query.lastError().text());
        setError(DatabaseError);
        emitResult();
        return;
    }

    auto mkdirJob = KIO::mkdir(QUrl::fromLocalFile(m_dataDir + m_uuid));
    connect(mkdirJob, &KJob::finished, this, &UpdateDatabaseJob::onDataDirReady);
}

void UpdateDatabaseJob::onDataDirReady(KJob *job)
{
    if (job->error()) {
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }

    for (const MimeData &data : m_mimeDataList) {
        QSqlQuery query(*m_db);
        query.prepare(u"INSERT INTO aux (uuid, mimetype, data_uuid) VALUES (?, ?, ?)"_s);
        query.addBindValue(m_uuid);
        query.addBindValue(data.type);
        query.addBindValue(data.uuid);
        if (query.exec()) {
            KIO::StoredTransferJob *fileJob =
                KIO::storedPut(data.data, QUrl::fromLocalFile(QString(m_dataDir + m_uuid + u'/' + data.uuid)), -1, KIO::HideProgressInfo);
            addSubjob(fileJob);
        }
    }
}

void UpdateDatabaseJob::slotResult(KJob *job)
{
    removeSubjob(job);

    if (hasSubjobs()) {
        return;
    }

    emitResult();
}

#include "moc_updateclipboardjob.cpp"
