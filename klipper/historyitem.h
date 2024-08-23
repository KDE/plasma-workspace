/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QFlags>
#include <QString>

#include "klipper_export.h"

class HistoryModel;
class QString;
class QMimeData;
class QDataStream;
class QSqlQuery;

class HistoryItem;
typedef std::shared_ptr<HistoryItem> HistoryItemPtr;
typedef std::shared_ptr<const HistoryItem> HistoryItemConstPtr;

enum class HistoryItemType {
    Unknown = 1 << 0,
    Text = 1 << 1,
    Image = 1 << 2,
    Url = 1 << 3,
};
Q_DECLARE_FLAGS(HistoryItemTypes, HistoryItemType);

/**
 * An entry in the clipboard history.
 */
class KLIPPER_EXPORT HistoryItem
{
public:
    explicit HistoryItem(QString &&uuid, QStringList &&mimeTypes, QString &&text);
    virtual ~HistoryItem();

    /**
     * Returns the primary item type.
     */
    HistoryItemType type() const;

    /**
     * Returns all the item types the item owns.
     */
    HistoryItemTypes allTypes() const
    {
        return m_types;
    }

    /**
     * Return the current item as text
     * An image would be returned as a descriptive
     * text, such as 32x43 image.
     */
    QString text() const;

    /**
     * @return uuid of current item.
     */
    const QString &uuid() const
    {
        return m_uuid;
    }

    /**
     * Equality.
     */
    bool operator==(const HistoryItem &rhs) const
    {
        return m_uuid == rhs.m_uuid;
    }

    /**
     * Create an HistoryItem from data stream (i.e., disk file)
     * returns null if creation fails. In this case, the datastream
     * is left in an undefined state.
     */
    static HistoryItemPtr create(const QSqlQuery &query);

private:
    QString m_uuid;
    HistoryItemTypes m_types = HistoryItemType::Unknown;
    QString m_text;
};
