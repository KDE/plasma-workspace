/*
    SPDX-FileCopyrightText: 2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <Plasma/DataContainer>

#include "playercontainer.h"

#include <QWeakPointer>

class Multiplexer : public Plasma::DataContainer
{
    Q_OBJECT

public:
    static const QLatin1String sourceName;

    explicit Multiplexer(QObject *parent = nullptr);

    void addPlayer(PlayerContainer *container);
    void removePlayer(const QString &name);
    PlayerContainer *activePlayer() const;

Q_SIGNALS:
    void activePlayerChanged(PlayerContainer *container);

    /**
     * There is no player opened.
     *
     * @since 5.24
     */
    void playerListEmptied();

private Q_SLOTS:
    void playerUpdated(const QString &name, const Plasma::DataEngine::Data &data);

private:
    void evaluatePlayer(PlayerContainer *container);
    void setBestActive();
    void replaceData(const Plasma::DataEngine::Data &data);
    PlayerContainer *firstPlayerFromHash(const QHash<QString, PlayerContainer *> &hash, PlayerContainer **proxyCandidate) const;

    QString m_activeName;
    QHash<QString, PlayerContainer *> m_playing;
    QHash<QString, PlayerContainer *> m_paused;
    QHash<QString, PlayerContainer *> m_stopped;

    QHash<qlonglong, PlayerContainer *> m_proxies;
};
