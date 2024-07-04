/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QByteArray>
#include <QHash>
#include <QObject>

#include "klipper_export.h"

class HistoryItem;
class HistoryModel;
class QAction;

class KLIPPER_EXPORT History : public QObject
{
    Q_OBJECT
public:
    explicit History(QObject *parent);
    ~History() override;

    /**
     * Inserts item into clipboard history top
     * if duplicate entry exist, the older duplicate is deleted.
     * The duplicate concept is "deep", so that two text string
     * are considerd duplicate if identical.
     */
    void insert(const std::shared_ptr<HistoryItem> &item);

    /**
     * Remove (first) history item equal to item from history
     */
    void remove(const std::shared_ptr<const HistoryItem> &item);

    /**
     * Traversal: Get first item
     */
    std::shared_ptr<const HistoryItem> first() const;

    /**
     * Get item identified by uuid
     */
    std::shared_ptr<const HistoryItem> find(const QByteArray &uuid) const;

    /**
     * @return next item in cycle, or null if at end
     */
    std::shared_ptr<const HistoryItem> nextInCycle() const;

    /**
     * @return previous item in cycle, or null if at top
     */
    std::shared_ptr<const HistoryItem> prevInCycle() const;

    /**
     * True if no history items
     */
    bool empty() const;

    /**
     * Set maximum history size
     */
    void setMaxSize(unsigned max_size);

    /**
     * Get the maximum history size
     */
    unsigned maxSize() const;

    /**
     * returns true if the user has selected the top item
     */
    bool topIsUserSelected()
    {
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
    void slotMoveToTop(QAction *action);

    /**
     * move the history in position pos to top
     */
    void slotMoveToTop(const QByteArray &uuid);

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

    std::shared_ptr<HistoryModel> m_model;

    QByteArray m_cycleStartUuid;
};
