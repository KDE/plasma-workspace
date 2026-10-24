/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "updateclipboardjob.h"
#include "mimetypes.h"
#include "utils.h"

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
#include <algorithm>

using namespace Qt::StringLiterals;

namespace
{
constexpr std::array<QStringView, 3> s_acceptableTextFormatPrefixes{
    u"text/",
    u"application/json",
    u"application/xml",
};
constexpr auto s_uriList = "text/uri-list"_L1;
constexpr auto s_qImageFormat = "application/x-qt-image"_L1;
constexpr auto s_imageFormatPrefix = "image/"_L1;
constexpr auto s_suggestedFileNameFormat = "application/x-kde-suggestedfilename"_L1;
}

constexpr bool hasAcceptableTextFormatPrefix(QStringView format)
{
    return std::ranges::any_of(s_acceptableTextFormatPrefixes, [format](QStringView prefix) {
        return format.startsWith(prefix);
    });
}

UpdateDatabaseJob *UpdateDatabaseJob::updateClipboard(QObject *parent,
                                                      QSqlDatabase *database,
                                                      QStringView databaseFolder,
                                                      const QString &uuid,
                                                      const QString &text,
                                                      const QMimeData *mimeData,
                                                      qreal timestamp)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    std::list<MimeData> mimeDataList;
    const QStringList formats = mimeData->formats();
    const auto begin = formats.cbegin();
    const auto end = formats.cend();
    // Formats that require special logic
    const auto foundIterators = Utils::find(formats,
                                            MimeTypes::plainText,
                                            MimeTypes::plainUtf8Text,
                                            MimeTypes::uriList,
                                            MimeTypes::png,
                                            MimeTypes::qtImage,
                                            MimeTypes::kdeSuggestedFileName);
    const auto [plainTextIt, plainUtf8TextIt, uriListIt, pngIt, qtImageIt, suggestedFilenameIt] = foundIterators;

    bool isUriListSet = false;
    bool isPlainTextSet = false;
    bool isPngSet = false;
    const bool usePng = pngIt != end;
    for (auto it = begin; it != end; ++it) {
        const auto &format = *it;
        if (!format.contains(u'/')) {
            continue;
        }

        if (it == uriListIt) {
            if (isUriListSet) {
                continue;
            }
            isUriListSet = true;
            QByteArray data = mimeData->data(format);
            hash.reset();
            hash.addData(data);
            mimeDataList.emplace_back(format, std::move(data), QString::fromLatin1(hash.result().toHex()));
            continue;
        }

        const bool isPlainText = it == plainTextIt || it == plainUtf8TextIt;
        if (isPlainText) {
            if (isPlainTextSet) {
                continue;
            }
            isPlainTextSet = true;
            QByteArray data = mimeData->data(format);
            hash.reset();
            hash.addData(data);
            mimeDataList.emplace_back(s_plainTextPrefix, std::move(data), QString::fromLatin1(hash.result().toHex()));
            mimeDataList.emplace_back(s_plainUtf8Text, QByteArray() /*Same uuid*/, QString::fromLatin1(hash.result().toHex()));
            continue;
        }

        const bool isImage = it == qtImageIt || format.startsWith(s_imageFormatPrefix);
        if (isImage) {
            if (isPngSet) {
                continue;
            }
            isPngSet = true;
            QByteArray data;
            QImage image;
            // If png is available directly do not do a needless encoding
            if (usePng) {
                data = mimeData->data(s_imageFormat);
                image = QImage::fromData(data, "PNG");
            } else {
                image = mimeData->imageData().value<QImage>();
                QBuffer buffer(&data);
                QImageWriter encoder(&buffer, "PNG");
                encoder.write(image);
            }
            auto imageHashBytes = reinterpret_cast<const char *>(image.constBits());
            hash.reset();
            hash.addData({imageHashBytes, image.sizeInBytes()});
            mimeDataList.emplace_back(s_imageFormat, std::move(data), QString::fromLatin1(hash.result().toHex()));
            continue;
        }

        // Keep the suggested file name.
        if (it != suggestedFilenameIt && !hasAcceptableTextFormatPrefix(format)) {
            // Don't create un-asked for DDE links in LibreOffice apps;
            // we don't want them.
            continue;
        }

        QByteArray data = mimeData->data(format);
        if (data.size() > 20 * 1000 * 1000) {
            // Skip anything greater than 20MB because we don't want too
            // many heavy things to be persistently held in the clipboard.
            continue;
        }

        hash.reset();
        hash.addData(data);
        mimeDataList.emplace_back(format, std::move(data), QString::fromLatin1(hash.result().toHex()));
    }

    // put fallback plain text for urls after original formats
    if (!isPlainTextSet && isUriListSet) {
        QByteArray data = mimeData->text().toUtf8(); // also returns urls
        hash.reset();
        hash.addData(data);
        mimeDataList.emplace_back(s_plainTextPrefix, std::move(data), QString::fromLatin1(hash.result().toHex()));
        mimeDataList.emplace_back(s_plainUtf8Text, QByteArray() /*Same uuid*/, QString::fromLatin1(hash.result().toHex()));
    }

    return new UpdateDatabaseJob(parent, database, databaseFolder, uuid, text, std::move(mimeDataList), timestamp);
}

UpdateDatabaseJob::UpdateDatabaseJob(QObject *parent,
                                     QSqlDatabase *database,
                                     QStringView databaseFolder,
                                     const QString &uuid,
                                     const QString &text,
                                     std::list<MimeData> &&mimeDataList,
                                     qreal timestamp)
    : KCompositeJob(parent)
    , m_db(database)
    , m_uuid(uuid)
    , m_text(text)
    , m_dataDir(databaseFolder + u"/data/")
    , m_mimeDataList(std::move(mimeDataList))
    , m_timestamp(timestamp)
{
}

UpdateDatabaseJob::~UpdateDatabaseJob() = default;

void UpdateDatabaseJob::start()
{
    if (!m_db->isOpen()) [[unlikely]] {
        setErrorText(u"Invalid database"_s);
        setError(DatabaseError);
        emitResult();
        return;
    }

    QSqlQuery query(*m_db);
    query.prepare(u"INSERT INTO main (uuid, added_time, last_used_time, mimetypes, text, starred) VALUES (?, ?, ?, ?, ?, ?)"_s);
    query.addBindValue(m_uuid);
    if (m_timestamp == 0) [[likely]] {
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
        query.addBindValue(QDateTime::currentMSecsSinceEpoch() / 1000.0);
    } else {
        query.addBindValue(qreal(m_timestamp));
        query.addBindValue(qreal(m_timestamp));
    }
    query.addBindValue(std::accumulate(std::next(m_mimeDataList.begin()),
                                       m_mimeDataList.end(),
                                       m_mimeDataList.begin()->type,
                                       [](const QString &a, const MimeData &b) -> QString {
                                           return a + u',' + b.type;
                                       }));
    query.addBindValue(m_text);
    query.addBindValue(false); // New items are not starred by default
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

    QSet<QStringView> addedUuidList;
    for (const MimeData &data : m_mimeDataList) {
        QSqlQuery query(*m_db);
        query.prepare(u"INSERT INTO aux (uuid, mimetype, data_uuid) VALUES (?, ?, ?)"_s);
        query.addBindValue(m_uuid);
        query.addBindValue(data.type);
        query.addBindValue(data.uuid);
        if (query.exec()) {
            // Different mimetypes can refer to the same content. Often happens between text/plain;charset=utf-8 and text/plain
            if (QStringView uuid(data.uuid); !addedUuidList.contains(uuid)) {
                addedUuidList.insert(uuid);
                KIO::StoredTransferJob *fileJob =
                    KIO::storedPut(data.data, QUrl::fromLocalFile(QString(m_dataDir + m_uuid + u'/' + data.uuid)), -1, KIO::HideProgressInfo);
                addSubjob(fileJob);
            }
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
