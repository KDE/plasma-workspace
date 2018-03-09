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
#ifndef HISTORYSTRINGITEM_H
#define HISTORYSTRINGITEM_H

#include <QMimeData>

#include "historyitem.h"

/**
 * A string entry in the clipboard history.
 */
class HistoryStringItem : public HistoryItem
{
public:
    explicit HistoryStringItem( const QString& data );
    ~HistoryStringItem() override {}
    QString text() const override;
    bool operator==( const HistoryItem& rhs) const override {
        if ( const HistoryStringItem* casted_rhs = dynamic_cast<const HistoryStringItem*>( &rhs ) ) {
            return casted_rhs->m_data == m_data;
        }
        return false;
    }
    QMimeData* mimeData() const override;

    /**
     * Write object on datastream
     */
    void write( QDataStream& stream ) const override;

private:
    QString m_data;
};

inline QString HistoryStringItem::text() const { return m_data; }

#endif
