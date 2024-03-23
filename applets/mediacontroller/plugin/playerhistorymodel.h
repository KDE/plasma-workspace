/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>

#include <KSharedConfig>

class PlayerHistoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum {
        IdentityRole = Qt::UserRole + 1,
    };

    explicit PlayerHistoryModel(QObject *parent = nullptr);
    ~PlayerHistoryModel() override;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Q_INVOKABLE int indexOf(const QString &identity) const;

    Q_INVOKABLE void rememberPlayer(const QString &identity);
    Q_INVOKABLE void forgetAllPlayers();

private:
    struct PlayerInformation {
        QString displayName;
        QString icon;
        QString identity;

        bool operator==(const QString &other) const
        {
            return identity == other;
        }
    };
    PlayerInformation readPlayer(const QString &identity) const;
    QList<PlayerInformation> readPlayers();

    QList<PlayerInformation> m_data;

    KSharedConfigPtr m_config;
    bool m_ready = false;
};
