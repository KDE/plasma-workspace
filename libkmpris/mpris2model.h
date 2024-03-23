

/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QConcatenateTablesProxyModel>
#include <QQmlParserStatus>
#include <qqmlregistration.h>

#include "kmpris_export.h"

class Mpris2FilterProxyModel;
class MultiplexerModel;
class PlayerContainer;

/**
 * A model that concatenates the multiplexer and players
 */
class KMPRIS_EXPORT Mpris2Model : public QConcatenateTablesProxyModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    QML_ELEMENT

    // The current index will always follow a player once set until the player is closed,
    // which means the current index is updated automatically.
    Q_PROPERTY(unsigned currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(PlayerContainer *currentPlayer READ currentPlayer NOTIFY currentPlayerChanged)

    /*
     * When a preferred player is set, the multiplexer will set the player as the active player
     * regardless of the playback status if it's available
     * @since 6.1
     */
    Q_PROPERTY(QString preferredPlayer READ default WRITE default NOTIFY preferredPlayerChanged BINDABLE bindablePreferredPlayer)

    /*
     * Whether the multiplexer should be added to the model
     * @since 6.1
     */
    Q_PROPERTY(bool multiplexerEnabled READ multiplexerEnabled WRITE setMultiplexerEnabled NOTIFY multiplexerEnabledChanged)

public:
    explicit Mpris2Model(QObject *parent = nullptr);
    ~Mpris2Model() override;

    QHash<int, QByteArray> roleNames() const override;

    unsigned currentIndex() const;
    void setCurrentIndex(unsigned index);
    PlayerContainer *currentPlayer() const;

    QBindable<QString> bindablePreferredPlayer();

    bool multiplexerEnabled() const;
    void setMultiplexerEnabled(bool enabled);

    Q_INVOKABLE PlayerContainer *playerForLauncherUrl(const QUrl &launcherUrl, unsigned pid) const;

    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void currentIndexChanged();
    void currentPlayerChanged();
    void preferredPlayerChanged();
    void multiplexerEnabledChanged();

private Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

private:
    void init();

    bool m_componentReady = false;
    std::list<QMetaObject::Connection> m_connectedSignals;

    std::shared_ptr<Mpris2FilterProxyModel> m_mprisModel;
    PlayerContainer *m_currentPlayer = nullptr;
    unsigned m_currentIndex = 0;

    bool m_multiplexerEnabled = true;
    MultiplexerModel *m_multiplexerModel = nullptr;
    Q_OBJECT_BINDABLE_PROPERTY(Mpris2Model, QString, m_preferredPlayer, &Mpris2Model::preferredPlayerChanged)
};
