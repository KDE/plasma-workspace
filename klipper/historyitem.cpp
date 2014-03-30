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
#include "historyitem.h"

#include <QMap>
#include <QPixmap>

#include <KDebug>
#include <kurlmimedata.h>

#include "historystringitem.h"
#include "historyimageitem.h"
#include "historyurlitem.h"

HistoryItem::HistoryItem(const QByteArray& uuid) : m_uuid(uuid) {

}

HistoryItem::~HistoryItem() {

}

HistoryItem* HistoryItem::create( const QMimeData* data )
{
#if 0
    int i=0;
    foreach ( QString format, data->formats() ) {
        kDebug() << "format(" << i++ <<"): " << format;
    }
#endif
    if (data->hasUrls())
    {
        KUrlMimeData::MetaDataMap metaData;
        QList<QUrl> urls = KUrlMimeData::urlsFromMimeData(data, KUrlMimeData::PreferKdeUrls, &metaData);
        QByteArray bytes = data->data("application/x-kde-cutselection");
        bool cut = !bytes.isEmpty() && (bytes.at(0) == '1'); // true if 1
        return new HistoryURLItem(urls, metaData, cut);
    }
    if (data->hasText())
    {
        return new HistoryStringItem(data->text());
    }
    if (data->hasImage())
    {
        QImage image = qvariant_cast<QImage>(data->imageData());
        return new HistoryImageItem(QPixmap::fromImage(image));
    }

    return 0; // Failed.
}

HistoryItem* HistoryItem::create( QDataStream& dataStream ) {
    if ( dataStream.atEnd() ) {
        return 0;
    }
    QString type;
    dataStream >> type;
    if ( type == "url" ) {
        QList<QUrl> urls;
        QMap< QString, QString > metaData;
        int cut;
        dataStream >> urls;
        dataStream >> metaData;
        dataStream >> cut;
        return new HistoryURLItem( urls, metaData, cut );
    }
    if ( type == "string" ) {
        QString text;
        dataStream >> text;
        return new HistoryStringItem( text );
    }
    if ( type == "image" ) {
        QPixmap image;
        dataStream >> image;
        return new HistoryImageItem( image );
    }
    kWarning() << "Failed to restore history item: Unknown type \"" << type << "\"" ;
    return 0;
}



void HistoryItem::chain(HistoryItem* next)
{
    m_next_uuid = next->uuid();
    next->m_previous_uuid = uuid();
}

void HistoryItem::insertBetweeen(HistoryItem* prev, HistoryItem* next)
{
    if (prev && next) {
        prev->chain(this);
        chain(next);
    } else {
        Q_ASSERT(!prev && !next);
        // First item in chain
        m_next_uuid = m_uuid;
        m_previous_uuid = m_uuid;
    }
#if 0 // Extra checks, if anyone ever needs them
    Q_ASSERT(prev->uuid() == m_previous_uuid);
    Q_ASSERT(prev->next_uuid() == m_uuid);
    Q_ASSERT(next->previous_uuid() == m_uuid);
    Q_ASSERT(next->uuid() == m_next_uuid);
    Q_ASSERT(prev->uuid() != uuid());
    Q_ASSERT(next->uuid() != uuid());
#endif
}

