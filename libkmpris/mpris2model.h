

/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QConcatenateTablesProxyModel>

#include <qqmlregistration.h>

#include "kmpris_export.h"

class Mpris2FilterProxyModel;
class MultiplexerModel;
class PlayerContainer;

/**
 * A model that concatenates the multiplexer and players
 */
class KMPRIS_EXPORT Mpris2Model : public QConcatenateTablesProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    // The current index will always follow a player once set until the player is closed,
    // which means the current index is updated automatically.
    Q_PROPERTY(unsigned currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(PlayerContainer *currentPlayer READ currentPlayer NOTIFY currentPlayerChanged)

public:
    explicit Mpris2Model(QObject *parent = nullptr);
    ~Mpris2Model() override;

    QHash<int, QByteArray> roleNames() const override;

    unsigned currentIndex() const;
    void setCurrentIndex(unsigned index);
    PlayerContainer *currentPlayer() const;

    Q_INVOKABLE PlayerContainer *playerForLauncherUrl(const QUrl &launcherUrl, unsigned pid) const;

Q_SIGNALS:
    void currentIndexChanged();
    void currentPlayerChanged();

private Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

private:
    std::shared_ptr<MultiplexerModel> m_multiplexerModel;
    std::shared_ptr<Mpris2FilterProxyModel> m_mprisModel;
    PlayerContainer *m_currentPlayer = nullptr;
    unsigned m_currentIndex = 0;
};
