/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
    Mpris2Engine(QObject *parent, const QVariantList &args);

    Plasma::Service *serviceForSource(const QString &source) override;
    QStringList sources() const override;

protected:
    bool sourceRequestEvent(const QString &source) override;
    bool updateSourceEvent(const QString &source) override;

private Q_SLOTS:
    void serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void initialFetchFinished(PlayerContainer *container);
    void initialFetchFailed(PlayerContainer *container);
    void serviceNameFetchFinished(QDBusPendingCallWatcher *watcher);

private:
    void addMediaPlayer(const QString &serviceName, const QString &sourceName);
    void createMultiplexer();

    QPointer<Multiplexer> m_multiplexer;
};
