/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef HISTORYURLITEM_H
#define HISTORYURLITEM_H

#include <kurlmimedata.h>

#include "historyitem.h"

/**
 * An URL entry in the clipboard history.
 */
class HistoryURLItem : public HistoryItem
{
public:
    HistoryURLItem(const QList<QUrl>& urls, const KUrlMimeData::MetaDataMap &metaData, bool cut );
    QString text() const override;
    bool operator==( const HistoryItem& rhs) const override;
    QMimeData* mimeData() const override;

    /**
     * Write object on datastream
     */
    void write( QDataStream& stream ) const override;
private:
    QList<QUrl> m_urls;
    KUrlMimeData::MetaDataMap m_metaData;
    bool m_cut;
};

#endif
