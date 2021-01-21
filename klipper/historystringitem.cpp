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
#include "historystringitem.h"

#include <QCryptographicHash>

HistoryStringItem::HistoryStringItem(const QString &data)
    : HistoryItem(QCryptographicHash::hash(data.toUtf8(), QCryptographicHash::Sha1))
    , m_data(data)
{
}

/* virtual */
void HistoryStringItem::write(QDataStream &stream) const
{
    stream << QStringLiteral("string") << m_data;
}

QMimeData *HistoryStringItem::mimeData() const
{
    QMimeData *data = new QMimeData();
    data->setText(m_data);
    return data;
}
