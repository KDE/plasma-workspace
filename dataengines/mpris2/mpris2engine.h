/*
 *   Copyright 2007-2012 Alex Merry <alex.merry@kdemail.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MPRIS2ENGINE_H
#define MPRIS2ENGINE_H

#include <Plasma/DataEngine>

#include <QWeakPointer>

class QDBusPendingCallWatcher;
class PlayerContainer;
class Multiplexer;

/**
 * The MPRIS2 data engine.
 */
class Mpris2Engine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    Mpris2Engine(QObject* parent, const QVariantList& args);

    Plasma::Service* serviceForSource(const QString& source) override;
    QStringList sources() const override;

protected:
    bool sourceRequestEvent(const QString& source) override;
    bool updateSourceEvent(const QString& source) override;

private Q_SLOTS:
    void serviceOwnerChanged(
            const QString& serviceName,
            const QString& oldOwner,
            const QString& newOwner);
    void initialFetchFinished(PlayerContainer* container);
    void initialFetchFailed(PlayerContainer* container);
    void serviceNameFetchFinished(QDBusPendingCallWatcher* watcher);

private:
    void addMediaPlayer(const QString& serviceName, const QString& sourceName);
    void createMultiplexer();

    QPointer<Multiplexer> m_multiplexer;
};

#endif // MPRIS2ENGINE_H
