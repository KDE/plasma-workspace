/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "historycycler.h"

#include "historyitem.h"
#include "historymodel.h"

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

HistoryCycler::HistoryCycler(QObject *parent)
    : QObject(parent)
    , m_model(HistoryModel::self())
{
    connect(m_model.get(), &HistoryModel::changed, [this](bool isTop) {
        if (isTop && !CycleBlocker::isBlocked()) {
            m_cycleStartUuid = QByteArray();
        }
    });
}

HistoryCycler::~HistoryCycler() = default;

void HistoryCycler::cycleNext()
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

void HistoryCycler::cyclePrev()
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

HistoryItemConstPtr HistoryCycler::nextInCycle() const
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

HistoryItemConstPtr HistoryCycler::prevInCycle() const
{
    if (m_cycleStartUuid.isEmpty()) {
        return HistoryItemConstPtr();
    }
    return m_model->index(m_model->rowCount() - 1).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
}

#include "moc_historycycler.cpp"
