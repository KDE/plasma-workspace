
/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QObject>

#include "kmpris_export.h"

class Mpris2FilterProxyModel;
class PlayerContainer;

class KMPRIS_EXPORT Multiplexer : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<Multiplexer> self();
    explicit Multiplexer(QObject *parent = nullptr);
    ~Multiplexer() override;

    /**
     * The player that is automatically chosen by the multiplexer.
     * When there is no player, the value is -1.
     */
    QBindable<int> activePlayerIndex() const;
    QBindable<PlayerContainer *> activePlayer() const;

Q_SIGNALS:
    void activePlayerIndexChanged(int newIndex);

private Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onPlaybackStatusChanged();

private:
    void updateIndex();
    void evaluatePlayers();

    std::shared_ptr<Mpris2FilterProxyModel> m_filterModel;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Multiplexer, PlayerContainer *, m_activePlayer, nullptr)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Multiplexer, int, m_activePlayerIndex, -1, &Multiplexer::activePlayerIndexChanged)
};
