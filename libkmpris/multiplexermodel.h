
/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QBindable>
#include <QSortFilterProxyModel>

#include <qqmlregistration.h>

#include "kmpris_export.h"

class Multiplexer;
class PlayerContainer;

/**
 * A model to list the only player from the multiplexer
 *
 * @note Can't use QSortFilterProxyModel since QConcatenateTablesProxyModel gets confused when two items are from the same source index,
 * and the frontend can switch to a different player seamlessly by using dataChanged.
 */
class KMPRIS_EXPORT MultiplexerModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit MultiplexerModel(QObject *parent = nullptr);
    ~MultiplexerModel() override;

    /**
     * To override the desktop icon
     */
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QBindable<QString> preferredPlayer();

private Q_SLOTS:
    void updateActivePlayer();

private:
    Multiplexer *m_multiplexer = nullptr;
    PlayerContainer *m_activePlayer = nullptr;
    Q_OBJECT_BINDABLE_PROPERTY(MultiplexerModel, QString, m_preferredPlayer)
};
