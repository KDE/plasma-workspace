/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "historyitem.h"
#include "klipper_export.h"

/**
 * A image entry in the clipboard history.
 */
class KLIPPER_EXPORT HistoryImageItem : public HistoryItem
{
public:
    explicit HistoryImageItem(const QImage &image, const QString &mimetype, const QByteArray &data);
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
            return &casted_rhs->m_image == &m_image; // Not perfect, but better than nothing.
        }
        return false;
    }
    QImage image() const override;
    QMimeData *mimeData() const override;

    void write(QDataStream &stream) const override;

private:
    /**
     * Image decoration for this item.
     * Also image data when copying an unformatted image.
     */
    const QImage m_image;
    /**
     * MIME type when copying a formatted image.
     */
    const QString m_mimetype;
    /**
     * Image data when copying a formatted image.
     */
    const QByteArray m_data;
    /**
     * Cache for m_data's string representation
     */
    mutable QString m_text;
};
