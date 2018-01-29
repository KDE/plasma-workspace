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

class HistoryItem;
class HistoryModel;
class QAction;

class History : public QObject
{
    Q_OBJECT
public:
    explicit History( QObject* parent );
    ~History() override;

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    void insert(QSharedPointer<HistoryItem> item);

    /**
     * Inserts item into clipboard without any checks
     * Used when restoring a saved history and internally.
     * Don't use this unless you're reasonable certain
     * that no duplicates are introduced
     */
    void forceInsert(QSharedPointer<HistoryItem> item);

    /**
     * Remove (first) history item equal to item from history
     */
    void remove( const QSharedPointer<const HistoryItem> &item  );

    /**
     * Traversal: Get first item
     */
    QSharedPointer<const HistoryItem> first() const;

    /**
     * Get item identified by uuid
     */
    QSharedPointer<const HistoryItem> find(const QByteArray& uuid) const;

    /**
     * @return next item in cycle, or null if at end
     */
    QSharedPointer<const HistoryItem> nextInCycle() const;

    /**
     * @return previous item in cycle, or null if at top
     */
    QSharedPointer<const HistoryItem> prevInCycle() const;

    /**
     * True if no history items
     */
    bool empty() const;

    /**
     * Set maximum history size
     */
    void setMaxSize( unsigned max_size );

    /**
     * Get the maximum history size
     */
    unsigned maxSize() const;

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

    HistoryModel *model() {
        return m_model;
    }

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

    void topIsUserSelectedSet();

private:
    /**
     * True if the top is selected by the user
     */
    bool m_topIsUserSelected;

    HistoryModel *m_model;

    QByteArray m_cycleStartUuid;
};

#endif
