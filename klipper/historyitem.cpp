/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "historyitem.h"

#include "klipper_debug.h"
#include <QBuffer>
#include <QImageReader>
#include <QMap>
#include <QMimeDatabase>
#include <QMimeType>
#include <QRegularExpression>


#include <kurlmimedata.h>

#include "historyimageitem.h"
#include "historymodel.h"
#include "historystringitem.h"
#include "historyurlitem.h"

using namespace Qt::StringLiterals;

HistoryItem::HistoryItem(const QByteArray &uuid)
    : m_uuid(uuid)
{
}

HistoryItem::~HistoryItem()
{
}

HistoryItemPtr HistoryItem::create(const QMimeData *mimeData)
{
    if (mimeData->hasUrls()) {
        KUrlMimeData::MetaDataMap metaData;
        QList<QUrl> urls = KUrlMimeData::urlsFromMimeData(mimeData, KUrlMimeData::PreferKdeUrls, &metaData);
        if (urls.isEmpty()) {
            return HistoryItemPtr();
        }
        QByteArray bytes = mimeData->data(QStringLiteral("application/x-kde-cutselection"));
        bool cut = !bytes.isEmpty() && (bytes.at(0) == '1'); // true if 1
        return HistoryItemPtr(new HistoryURLItem(urls, metaData, cut));
    }
    if (mimeData->hasText()) {
        const QString text = mimeData->text();
        if (text.isEmpty()) { // reading mime data can fail. Avoid ghost entries
            return HistoryItemPtr();
        }
        return HistoryItemPtr(new HistoryStringItem(mimeData->text()));
    }
    if (mimeData->hasImage()) {
        const auto formats = mimeData->formats();
        qCDebug(KLIPPER_LOG) << "HAS_IMAGE" << formats;
        QImage image;
        QString mimetype;
        QByteArray data;
        // Use the first image format that has data.
        for (const auto &format : formats) {
            if (!format.startsWith(u"image/")) {
                continue;
            }
            data = mimeData->data(format);
            if (!data.isEmpty()) {
                mimetype = format;
                qCDebug(KLIPPER_LOG) << "IMAGE_MIMETYPE" << mimetype;
                qCDebug(KLIPPER_LOG).nospace() << "IMAGE_DATA\n" << data.first(std::min(256ll, data.size())) << "\nTRUNCATED TO 256 BYTES";
                break;
            }
        }
        if (!data.isEmpty()) {
            QBuffer buffer(&data);
            QImageReader reader(&buffer);
            image = reader.read();
        }
        if (image.isNull()) {
            qCDebug(KLIPPER_LOG) << "IMAGE_FROM_QMimeData::imageData";
            image = qvariant_cast<QImage>(mimeData->imageData());
        }
        if (image.isNull()) {
            qCDebug(KLIPPER_LOG) << "NULL_IMAGE";
            return HistoryItemPtr();
        }
        qCDebug(KLIPPER_LOG) << "IMAGE_DECORATION" << image;
        return HistoryItemPtr(new HistoryImageItem(image, mimetype, data));
    }

    return HistoryItemPtr(); // Failed.
}

HistoryItemPtr HistoryItem::create(QDataStream &dataStream)
{
    if (dataStream.atEnd()) {
        return HistoryItemPtr();
    }
    QString type;
    dataStream >> type;
    if (type == QLatin1String("url")) {
        QList<QUrl> urls;
        QMap<QString, QString> metaData;
        int cut;
        dataStream >> urls;
        dataStream >> metaData;
        dataStream >> cut;
        return HistoryItemPtr(new HistoryURLItem(urls, metaData, cut));
    }
    if (type == QLatin1String("string")) {
        QString text;
        dataStream >> text;
        return HistoryItemPtr(new HistoryStringItem(text));
    }
    if (type == QLatin1String("image")) {
        QImage image;
        dataStream >> image;
        return HistoryItemPtr(new HistoryImageItem(image, {}, {}));
    }
    qCWarning(KLIPPER_LOG) << "Failed to restore history item: Unknown type \"" << type << "\"";
    return HistoryItemPtr();
}

#include "moc_historyitem.cpp"
