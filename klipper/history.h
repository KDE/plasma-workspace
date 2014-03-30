/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>
   Copyright (C) Andrew Stanley-Jones <asj@cban.com>

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
#ifndef HISTORY_H
#define HISTORY_H

#include <QObject>
#include <QHash>
#include <QByteArray>

#include "historyitem.h"

class QAction;
class KlipperPopup;

class History : public QObject
{
    Q_OBJECT
public:
    History( QObject* parent );
    ~History();

    /**
     * Return (toplevel) popup menu (or default view, of you like)
     */
    KlipperPopup* popup();

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    void insert( HistoryItem* item );

    /**
     * Inserts item into clipboard without any checks
     * Used when restoring a saved history and internally.
     * Don't use this unless you're reasonable certain
     * that no duplicates are introduced
     */
    void forceInsert( HistoryItem* item );

    /**
     * Remove (first) history item equal to item from history
     */
    void remove( const HistoryItem* item  );

    /**
     * Traversal: Get first item
     */
    const HistoryItem* first() const;

    /**
     * Get item identified by uuid
     */
    const HistoryItem* find(const QByteArray& uuid) const;

    /**
     * @return next item in cycle, or null if at end
     */
    const HistoryItem* nextInCycle() const;

    /**
     * @return previous item in cycle, or null if at top
     */
    const HistoryItem* prevInCycle() const;

    /**
     * True if no history items
     */
    bool empty() const { return m_items.isEmpty(); }

    /**
     * Set maximum history size
     */
    void setMaxSize( unsigned max_size );

    /**
     * Get the maximum history size
     */
    unsigned maxSize() const { return m_maxSize; }

    /**
     * returns true if the user has selected the top item
     */
    bool topIsUserSelected() {
        return m_topIsUserSelected;
    }

    /**
     * Cycle to next item
     */
    void cycleNext();

    /**
     * Cycle to prev item
     */
    void cyclePrev();

public Q_SLOTS:
    /**
     * move the history in position pos to top
     */
    void slotMoveToTop(QAction* action);

    /**
     * move the history in position pos to top
     */
    void slotMoveToTop(const QByteArray& uuid);

    /**
     * Clear history
     */
    void slotClear();

Q_SIGNALS:
    void changed();

    /**
     * Emitted when the first history item has changed.
     */
    void topChanged();

private:
    /**
     * ensure that the number of items does not exceed max_size()
     * Deletes items from the end as necessary.
     */
    void trim();

private:
    typedef QHash<QByteArray, HistoryItem*> items_t;
    /**
     * The history
     */
    items_t m_items;

    /**
     * First item
     */
    HistoryItem* m_top;

    /**
     * "Default view" --- a popupmenu containing the clipboard history.
     */
    KlipperPopup* m_popup;


    /**
     * The number of clipboard items stored.
     */
    unsigned m_maxSize;

    /**
     * True if the top is selected by the user
     */
    bool m_topIsUserSelected;

    /**
     * The "next" when cycling through the
     * history. May be 0, if history is empty
     */
    HistoryItem* m_nextCycle;
};

inline const HistoryItem* History::first() const { return m_top; }

#endif
