/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QPixmap>

class HistoryModel;
class QString;
class QMimeData;
class QDataStream;

class HistoryItem;
typedef QSharedPointer<HistoryItem> HistoryItemPtr;
typedef QSharedPointer<const HistoryItem> HistoryItemConstPtr;

enum class HistoryItemType {
    Text,
    Image,
    Url,
};

/**
 * An entry in the clipboard history.
 */
class HistoryItem
{
public:
    explicit HistoryItem(const QByteArray &uuid);
    virtual ~HistoryItem();

    /**
     * Returns the item type.
     */
    virtual HistoryItemType type() const = 0;

    /**
     * Return the current item as text
     * An image would be returned as a descriptive
     * text, such as 32x43 image.
     */
    virtual QString text() const = 0;

    /**
     * @return uuid of current item.
     */
    const QByteArray &uuid() const
    {
        return m_uuid;
    }

    /**
     * Return the current item as pixmap
     * A text would be returned as a null pixmap,
     * which is also the default implementation
     */
    inline virtual QPixmap image() const;

    /**
     * Returns a pointer to a QMimeData suitable for QClipboard::setMimeData().
     */
    virtual QMimeData *mimeData() const = 0;

    /**
     * Write object on datastream
     */
    virtual void write(QDataStream &stream) const = 0;

    /**
     * Equality.
     */
    virtual bool operator==(const HistoryItem &rhs) const = 0;

    /**
     * Create an HistoryItem from MimeSources (i.e., clipboard data)
     * returns null if create fails (e.g, unsupported mimetype)
     */
    static HistoryItemPtr create(const QMimeData *data);

    /**
     * Create an HistoryItem from data stream (i.e., disk file)
     * returns null if creation fails. In this case, the datastream
     * is left in an undefined state.
     */
    static HistoryItemPtr create(QDataStream &dataStream);

    /**
     * previous item's uuid
     * TODO: drop, only used in unit test now
     */
    QByteArray previous_uuid() const;

    /**
     * next item's uuid
     * TODO: drop, only used in unit test now
     */
    QByteArray next_uuid() const;

    void setModel(HistoryModel *model);

protected:
    HistoryModel *m_model;

private:
    QByteArray m_uuid;
};

inline QPixmap HistoryItem::image() const
{
    return QPixmap();
}

inline QDataStream &operator<<(QDataStream &lhs, HistoryItem const *const rhs)
{
    if (rhs) {
        rhs->write(lhs);
    }
    return lhs;
}

Q_DECLARE_METATYPE(HistoryItem *)
Q_DECLARE_METATYPE(HistoryItemPtr)
Q_DECLARE_METATYPE(HistoryItemConstPtr)
Q_DECLARE_METATYPE(HistoryItemType)
