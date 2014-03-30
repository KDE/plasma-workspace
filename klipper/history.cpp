/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>
   Copyright (C) by Andrew Stanley-Jones <asj@cban.com>
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>

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
#include "history.h"

#include <QAction>

#include <KDebug>

#include "historystringitem.h"
#include "klipperpopup.h"

History::History( QObject* parent )
    : QObject( parent ),
      m_top(0L),
      m_popup( new KlipperPopup( this ) ),
      m_topIsUserSelected( false ),
      m_nextCycle(0L)
{
    connect( this, SIGNAL(changed()), m_popup, SLOT(slotHistoryChanged()) );

}


History::~History() {
    qDeleteAll(m_items);
}

void History::insert( HistoryItem* item ) {
    if ( !item )
        return;

    m_topIsUserSelected = false;
    const HistoryItem* existingItem = this->find(item->uuid());
    if ( existingItem ) {
        if ( existingItem == m_top) {
            return;
        }
        slotMoveToTop( existingItem->uuid() );
    } else {
        forceInsert( item );
    }

    emit topChanged();

}

void History::forceInsert( HistoryItem* item ) {
    if ( !item )
        return;
    if (m_items.find(item->uuid()) != m_items.end()) {
        return; // Don't insert duplicates
    }
    m_nextCycle = m_top;
    item->insertBetweeen(m_top ? m_items[m_top->previous_uuid()] : 0L, m_top);
    m_items.insert( item->uuid(), item );
    m_top = item;
    emit changed();
    trim();
}

void History::trim() {
    int i = m_items.count() - maxSize();
    if ( i <= 0 || !m_top )
        return;

    items_t::iterator bottom = m_items.find(m_top->previous_uuid());
    while ( i-- ) {
        items_t::iterator it = bottom;
        bottom = m_items.find((*bottom)->previous_uuid());
        // FIXME: managing memory manually is tedious; use smart pointer instead
        delete *it;
        m_items.erase(it);
    }
    (*bottom)->chain(m_top);
    if (m_items.size()<=1) {
        m_nextCycle = 0L;
    }
    emit changed();
}

void History::remove( const HistoryItem* newItem ) {
    if ( !newItem )
        return;

    items_t::iterator it = m_items.find(newItem->uuid());
    if (it == m_items.end()) {
        return;
    }

    if (*it == m_top) {
        m_top = m_items[m_top->next_uuid()];
    }
    m_items[(*it)->previous_uuid()]->chain(m_items[(*it)->next_uuid()]);
    m_items.erase(it);
}


void History::slotClear() {
    // FIXME: managing memory manually is tedious; use smart pointer instead
    qDeleteAll(m_items);
    m_items.clear();
    m_top = 0L;
    emit changed();
}

void History::slotMoveToTop(QAction* action) {
    QByteArray uuid = action->data().toByteArray();
    if (uuid.isNull()) // not an action from popupproxy
        return;

    slotMoveToTop(uuid);
}

void History::slotMoveToTop(const QByteArray& uuid) {

    items_t::iterator it = m_items.find(uuid);
    if (it == m_items.end()) {
        return;
    }
    if (*it == m_top) {
        emit topChanged();
        return;
    }
    m_topIsUserSelected = true;

    m_nextCycle = m_top;
    m_items[(*it)->previous_uuid()]->chain(m_items[(*it)->next_uuid()]);
    (*it)->insertBetweeen(m_items[m_top->previous_uuid()], m_top);
    m_top = *it;
    emit changed();
    emit topChanged();
}

void History::setMaxSize( unsigned max_size ) {
    m_maxSize = max_size;
    trim();
}

KlipperPopup* History::popup() {
    return m_popup;
}

void History::cycleNext() {
    if (m_top && m_nextCycle && m_nextCycle != m_top) {
        HistoryItem* prev = m_items[m_nextCycle->previous_uuid()];
        HistoryItem* next = m_items[m_nextCycle->next_uuid()];
        //if we have only two items in clipboard
        if (prev == next) {
            m_top=m_nextCycle;
        }
        else {
            HistoryItem* endofhist = m_items[m_top->previous_uuid()];
            HistoryItem* aftertop = m_items[m_top->next_uuid()];
            if (prev == m_top) {
                prev = m_nextCycle;
                aftertop = m_top;
            }
            else if (next == m_top) {
                next = m_nextCycle;
                endofhist = m_top;
            }
            m_top->insertBetweeen(prev, next);
            m_nextCycle->insertBetweeen(endofhist, aftertop);
            m_top = m_nextCycle;
            m_nextCycle = next;
        }
        emit changed();
        emit topChanged();
    }
}

void History::cyclePrev() {
    if (m_top && m_nextCycle) {
        HistoryItem* prev = m_items[m_nextCycle->previous_uuid()];
        if (prev == m_top) {
            return;
        }
        HistoryItem* prevprev = m_items[prev->previous_uuid()];
        HistoryItem* aftertop = m_items[m_top->next_uuid()];
        //if we have only two items in clipboard
        if (m_nextCycle == prevprev) {
            m_top=aftertop;
        }
        else {
            HistoryItem* endofhist = m_items[m_top->previous_uuid()];
            if (prevprev == m_top) {
                prevprev = prev;
                aftertop = m_top;
            }
            else if (m_nextCycle == m_top) {
                m_nextCycle = aftertop;
                endofhist = m_top;
            }
            m_top->insertBetweeen(prevprev,m_nextCycle);
            prev->insertBetweeen(endofhist, aftertop);
            m_nextCycle = m_top;
            m_top = prev;
        }
        emit changed();
        emit topChanged();
    }
}


const HistoryItem* History::nextInCycle() const
{
    return m_nextCycle != m_top ? m_nextCycle : 0L; // pointing to top=no more items

}

const HistoryItem* History::prevInCycle() const
{
    if (m_nextCycle) {
        const HistoryItem* prev = m_items[m_nextCycle->previous_uuid()];
        if (prev != m_top) {
            return prev;
        }
    }
    return 0L;

}

const HistoryItem* History::find(const QByteArray& uuid) const
{
    items_t::const_iterator it = m_items.find(uuid);
    return (it == m_items.end()) ? 0L : *it;
}

#include "history.moc"
