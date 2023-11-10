/*
    SPDX-FileCopyrightText: 2007-2012 Alex Merry <alex.merry@kdemail.net>
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <unordered_map>
#include <vector>

#include <QAbstractListModel>
#include <QConcatenateTablesProxyModel>

#include "kmpris_export.h"

class QDBusPendingCallWatcher;
class Multiplexer;
class PlayerContainer;

/**
 * The MPRIS2 data model where player information is stored and monitored
 */
class KMPRIS_NO_EXPORT Mpris2SourceModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        TrackRole = Qt::DisplayRole,
        ContainerRole = Qt::UserRole + 1,
        CanControlRole,
        CanGoNextRole,
        CanGoPreviousRole,
        CanPlayRole,
        CanPauseRole,
        CanStopRole,
        CanSeekRole,
        LoopStatusRole,
        PlaybackStatusRole,
        PositionRole,
        RateRole,
        ShuffleRole,
        VolumeRole,
        ArtUrlRole,
        ArtistRole,
        AlbumRole,
        LengthRole,
        CanQuitRole,
        CanRaiseRole,
        CanSetFullscreenRole,
        DesktopEntryRole,
        IdentityRole,
        IconNameRole,
        InstancePidRole,
        KDEPidRole,
    };
    Q_ENUM(Role)

    static std::shared_ptr<Mpris2SourceModel> self();
    ~Mpris2SourceModel() override;

    static QVariant dataFromPlayer(PlayerContainer *container, int role);
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void onServiceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner);
    void onServiceNamesFetched(QDBusPendingCallWatcher *watcher);
    void onInitialFetchFinished(PlayerContainer *container);
    void onInitialFetchFailed(PlayerContainer *container);

private:
    Mpris2SourceModel(QObject *parent = nullptr);

    void fetchServiceNames();
    void addMediaPlayer(const QString &serviceName, const QString &sourceName);
    void removeMediaPlayer(const QString &sourceName);

    std::vector<PlayerContainer *> m_containers;
    std::unordered_map<QString, PlayerContainer *> m_pendingContainers;

    friend class Multiplexer;
};