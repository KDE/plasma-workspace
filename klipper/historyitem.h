/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QMap>
#include <QPixmap>

#include "klipper_export.h"

class HistoryModel;
class QString;
class QMimeData;
class QDataStream;
class QUrl;

class HistoryItem;
typedef std::unique_ptr<HistoryItem> HistoryItemPtr;
typedef std::shared_ptr<const HistoryItem> HistoryItemSharedPtr;

using MimeDataMap = QVariantMap;
using QUrlList = QList<QUrl>;

enum class HistoryItemType {
    Invalid,
    Text,
    Image,
    Url,
};

/**
 * An entry in the clipboard history.
 */
class KLIPPER_EXPORT HistoryItem
{
public:
    // Only construct with HistoryItem::create externally
    explicit HistoryItem(const MimeDataMap &mimeDataMap, HistoryItemType type = HistoryItemType::Invalid, const QImage &image = {});
    ~HistoryItem();

    /**
     * Returns the item type.
     */
    HistoryItemType type() const;

    /**
     * Return the current item as text
     * An image would be returned as a descriptive
     * text, such as 32x43 image.
     */
    QString text() const;

    /**
     * @return uuid of current item.
     */
    const QByteArray &uuid() const
    {
        return m_uuid;
    }

    /**
     * Return the current item as \QImage
     * A text would be returned as a null \QImage,
     * which is also the default implementation
     */
    QImage image() const;

    /**
     * Returns a pointer to a QMimeData suitable for QClipboard::setMimeData().
     */
    QMimeData *newQMimeData() const;

    /**
     * Write object on datastream
     */
    void write(QDataStream &stream) const;

    /**
     * Equality.
     */
    bool operator==(const HistoryItem &rhs) const;

    /**
     * Create an HistoryItem from MimeSources (i.e., clipboard data)
     * returns null if create fails (e.g, unsupported mimetype)
     */
    static HistoryItemPtr create(const QMimeData *data, std::optional<QStringList> formats = {});

    /**
     * Create an HistoryItem from data stream (i.e., disk file)
     * returns null if creation fails. In this case, the datastream
     * is left in an undefined state.
     */
    static HistoryItemPtr create(QDataStream &dataStream);

    static HistoryItemPtr create(const QString &data);

    static HistoryItemPtr create(const QImage &data);

    static HistoryItemPtr create(const QUrlList &data);

private:

    MimeDataMap m_mimeDataMap;
    QByteArray m_uuid;
    HistoryItemType m_type;
    QString m_text;
    QImage m_image;
};

inline QDataStream &operator<<(QDataStream &lhs, HistoryItem const *const rhs)
{
    if (rhs) {
        rhs->write(lhs);
    }
    return lhs;
}
