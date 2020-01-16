/*
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

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

private Q_SLOTS:
    void playerUpdated(const QString &name, const Plasma::DataEngine::Data &data);

private:
    void evaluatePlayer(PlayerContainer *container);
    void setBestActive();
    void replaceData(const Plasma::DataEngine::Data &data);
    PlayerContainer *firstPlayerFromHash(const QHash<QString, PlayerContainer *> &hash, PlayerContainer **proxyCandidate) const;

    QString m_activeName;
    QHash<QString,PlayerContainer*> m_playing;
    QHash<QString,PlayerContainer*> m_paused;
    QHash<QString,PlayerContainer*> m_stopped;

    QHash<qlonglong, PlayerContainer *> m_proxies;
};

#endif // MULTIPLEXER_H
