/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <memory>

#include <QByteArray>
#include <QObject>

#include "klipper_export.h"

class HistoryItem;
class HistoryModel;
class QAction;

class KLIPPER_EXPORT HistoryCycler : public QObject
{
    Q_OBJECT
public:
    explicit HistoryCycler(QObject *parent);
    ~HistoryCycler() override;

    /**
     * @return next item in cycle, or null if at end
     */
    std::shared_ptr<const HistoryItem> nextInCycle() const;

    /**
     * @return previous item in cycle, or null if at top
     */
    std::shared_ptr<const HistoryItem> prevInCycle() const;

    /**
     * Cycle to next item
     */
    void cycleNext();

    /**
     * Cycle to prev item
     */
    void cyclePrev();

private:
    std::shared_ptr<HistoryModel> m_model;

    QByteArray m_cycleStartUuid;
};
