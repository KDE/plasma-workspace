/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <kurlmimedata.h>

#include "historyitem.h"

/**
 * An URL entry in the clipboard history.
 */
class HistoryURLItem : public HistoryItem
{
public:
    HistoryURLItem(const QList<QUrl> &urls, const KUrlMimeData::MetaDataMap &metaData, bool cut);

    HistoryItemType type() const override
    {
        return HistoryItemType::Url;
    }

    QString text() const override;
    bool operator==(const HistoryItem &rhs) const override;
    QMimeData *mimeData() const override;

    /**
     * Write object on datastream
     */
    void write(QDataStream &stream) const override;

private:
    QList<QUrl> m_urls;
    KUrlMimeData::MetaDataMap m_metaData;
    bool m_cut;
};
