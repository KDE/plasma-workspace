/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "history.h"

#include <QAction>

#include "historyitem.h"
#include "historymodel.h"
#include "historystringitem.h"

class CycleBlocker
{
public:
    CycleBlocker();
    ~CycleBlocker();

    static bool isBlocked();

private:
    static int s_blocker;
};

int CycleBlocker::s_blocker = 0;
CycleBlocker::CycleBlocker()
{
    s_blocker++;
}

CycleBlocker::~CycleBlocker()
{
    s_blocker--;
}

bool CycleBlocker::isBlocked()
{
    return s_blocker;
}

History::History(QObject *parent)
    : QObject(parent)
    , m_topIsUserSelected(false)
    , m_model(new HistoryModel(this))
{
    connect(m_model, &HistoryModel::rowsInserted, this, [this](const QModelIndex &parent, int start) {
        Q_UNUSED(parent)
        if (start == 0) {
            Q_EMIT topChanged();
        }
        Q_EMIT changed();
    });
    connect(m_model,
            &HistoryModel::rowsMoved,
            this,
            [this](const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow) {
                Q_UNUSED(sourceParent)
                Q_UNUSED(sourceEnd)
                Q_UNUSED(destinationParent)
                if (sourceStart == 0 || destinationRow == 0) {
                    Q_EMIT topChanged();
                }
                Q_EMIT changed();
            });
    connect(m_model, &HistoryModel::rowsRemoved, this, [this](const QModelIndex &parent, int start) {
        Q_UNUSED(parent)
        if (start == 0) {
            Q_EMIT topChanged();
        }
        Q_EMIT changed();
    });
    connect(m_model, &HistoryModel::modelReset, this, &History::changed);
    connect(m_model, &HistoryModel::modelReset, this, &History::topChanged);
    connect(this, &History::topChanged, [this]() {
        m_topIsUserSelected = false;
        if (!CycleBlocker::isBlocked()) {
            m_cycleStartUuid = QByteArray();
        }
    });
}

History::~History()
{
}

void History::insert(HistoryItemPtr item)
{
    if (!item)
        return;

    m_model->insert(item);
}

void History::forceInsert(HistoryItemPtr item)
{
    if (!item)
        return;
    // TODO: do we need a force insert in HistoryModel
    m_model->insert(item);
}

void History::remove(const HistoryItemConstPtr &newItem)
{
    if (!newItem)
        return;

    m_model->remove(newItem->uuid());
}

void History::slotClear()
{
    m_model->clear();
}

void History::slotMoveToTop(QAction *action)
{
    QByteArray uuid = action->data().toByteArray();
    if (uuid.isNull()) // not an action from popupproxy
        return;

    slotMoveToTop(uuid);
}

void History::slotMoveToTop(const QByteArray &uuid)
{
    const QModelIndex item = m_model->indexOf(uuid);
    if (item.isValid() && item.row() == 0) {
        // The item is already at the top, but it still may be not be set as the actual clipboard
        // contents, normally this happens if the item is only in the X11 mouse selection but
        // not in the Ctrl+V clipboard.
        Q_EMIT topChanged();
        m_topIsUserSelected = true;
        Q_EMIT topIsUserSelectedSet();
        return;
    }
    m_model->moveToTop(uuid);
    m_topIsUserSelected = true;
    Q_EMIT topIsUserSelectedSet();
}

void History::setMaxSize(unsigned max_size)
{
    m_model->setMaxSize(max_size);
}

void History::cycleNext()
{
    if (m_model->rowCount() < 2) {
        return;
    }

    if (m_cycleStartUuid.isEmpty()) {
        m_cycleStartUuid = m_model->index(0).data(HistoryModel::UuidRole).toByteArray();
    } else if (m_cycleStartUuid == m_model->index(1).data(HistoryModel::UuidRole).toByteArray()) {
        // end of cycle
        return;
    }
    CycleBlocker blocker;
    m_model->moveTopToBack();
}

void History::cyclePrev()
{
    if (m_cycleStartUuid.isEmpty()) {
        return;
    }
    CycleBlocker blocker;
    m_model->moveBackToTop();
    if (m_cycleStartUuid == m_model->index(0).data(HistoryModel::UuidRole).toByteArray()) {
        m_cycleStartUuid = QByteArray();
    }
}

HistoryItemConstPtr History::nextInCycle() const
{
    if (m_model->hasIndex(1, 0)) {
        if (!m_cycleStartUuid.isEmpty()) {
            // check whether we are not at the end
            if (m_cycleStartUuid == m_model->index(1).data(HistoryModel::UuidRole).toByteArray()) {
                return HistoryItemConstPtr();
            }
        }
        return m_model->index(1).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
    }
    return HistoryItemConstPtr();
}

HistoryItemConstPtr History::prevInCycle() const
{
    if (m_cycleStartUuid.isEmpty()) {
        return HistoryItemConstPtr();
    }
    return m_model->index(m_model->rowCount() - 1).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
}

HistoryItemConstPtr History::find(const QByteArray &uuid) const
{
    const QModelIndex index = m_model->indexOf(uuid);
    if (!index.isValid()) {
        return HistoryItemConstPtr();
    }
    return index.data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
}

bool History::empty() const
{
    return m_model->rowCount() == 0;
}

unsigned int History::maxSize() const
{
    return m_model->maxSize();
}

HistoryItemConstPtr History::first() const
{
    const QModelIndex index = m_model->index(0);
    if (!index.isValid()) {
        return HistoryItemConstPtr();
    }
    return index.data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
}
