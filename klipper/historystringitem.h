/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QMimeData>

#include "historyitem.h"

/**
 * A string entry in the clipboard history.
 */
class HistoryStringItem : public HistoryItem
{
public:
    explicit HistoryStringItem(const QString &data);
    ~HistoryStringItem() override
    {
    }

    HistoryItemType type() const override
    {
        return HistoryItemType::Text;
    }

    QString text() const override;
    bool operator==(const HistoryItem &rhs) const override
    {
        if (const HistoryStringItem *casted_rhs = dynamic_cast<const HistoryStringItem *>(&rhs)) {
            return casted_rhs->m_data == m_data;
        }
        return false;
    }
    QMimeData *mimeData() const override;

    /**
     * Write object on datastream
     */
    void write(QDataStream &stream) const override;

private:
    QString m_data;
};

inline QString HistoryStringItem::text() const
{
    return m_data;
}
