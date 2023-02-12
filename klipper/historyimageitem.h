/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "historyitem.h"

/**
 * A image entry in the clipboard history.
 */
class HistoryImageItem : public HistoryItem
{
public:
    explicit HistoryImageItem(const QImage &data);
    ~HistoryImageItem() override
    {
    }

    HistoryItemType type() const override
    {
        return HistoryItemType::Image;
    }

    QString text() const override;
    bool operator==(const HistoryItem &rhs) const override
    {
        if (const HistoryImageItem *casted_rhs = dynamic_cast<const HistoryImageItem *>(&rhs)) {
            return &casted_rhs->m_data == &m_data; // Not perfect, but better than nothing.
        }
        return false;
    }
    QPixmap image() const override;
    QMimeData *mimeData() const override;

    void write(QDataStream &stream) const override;

private:
    /**
     *
     */
    const QImage m_data;
    /**
     * Cache for m_data's string representation
     */
    mutable QString m_text;
};
