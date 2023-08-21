
/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QSortFilterProxyModel>

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

public:
    static std::shared_ptr<MultiplexerModel> self();
    explicit MultiplexerModel(QObject *parent = nullptr);
    ~MultiplexerModel() override;

    /**
     * To override the desktop icon
     */
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void updateActivePlayer();

private:
    std::shared_ptr<Multiplexer> m_multiplexer;
    PlayerContainer *m_activePlayer = nullptr;
};
