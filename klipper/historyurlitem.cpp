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
#include "historyurlitem.h"

#include <QMimeData>
#include <QCryptographicHash>

namespace {
    QByteArray compute_uuid(const QList<QUrl>& _urls, const KUrlMimeData::MetaDataMap &_metaData, bool _cut ) {
        QCryptographicHash hash(QCryptographicHash::Sha1);
        foreach(const QUrl& url, _urls) {
            hash.addData(url.toEncoded());
            hash.addData("\0", 1); // Use binary zero as that is not a valid path character
        }
        QByteArray buffer;
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << _metaData << "\0" << _cut;
        hash.addData(buffer);
        return hash.result();
    }
}

HistoryURLItem::HistoryURLItem( const QList<QUrl>& _urls, const KUrlMimeData::MetaDataMap &_metaData, bool _cut )
    : HistoryItem(compute_uuid(_urls, _metaData, _cut))
    , m_urls( _urls )
    , m_metaData( _metaData )
    , m_cut( _cut )
{
}

/* virtual */
void HistoryURLItem::write( QDataStream& stream ) const
{
    stream << QStringLiteral( "url" ) << m_urls << m_metaData << (int)m_cut;
}

QString HistoryURLItem::text() const {
    QString ret;
    bool first = true;
    for (const QUrl &url : m_urls) {
        if (!first) {
            ret.append(QLatin1Char(' '));
        }
        first = false;
        ret.append(url.toString(QUrl::FullyEncoded));
    }
    return ret;
}

QMimeData* HistoryURLItem::mimeData() const {
    QMimeData *data = new QMimeData();
    data->setUrls(m_urls);
    KUrlMimeData::setMetaData(m_metaData, data);
    data->setData(QStringLiteral("application/x-kde-cutselection"), QByteArray(m_cut ? "1" : "0"));
    return data;
}

bool HistoryURLItem::operator==( const HistoryItem& rhs) const
{
    if ( const HistoryURLItem* casted_rhs = dynamic_cast<const HistoryURLItem*>( &rhs ) ) {
        return casted_rhs->m_urls == m_urls
            && casted_rhs->m_metaData.count() == m_metaData.count()
            && std::equal( casted_rhs->m_metaData.begin(), casted_rhs->m_metaData.end(), m_metaData.begin())
            && casted_rhs->m_cut == m_cut;
    }
    return false;
}
