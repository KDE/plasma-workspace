

/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <unordered_set>

#include <QSortFilterProxyModel>

#include "kmpris_export.h"

class Mpris2SourceModel;

/**
 * A model that filters out duplicate players from Plasma Browser Integration
 */
class KMPRIS_NO_EXPORT Mpris2FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    static std::shared_ptr<Mpris2FilterProxyModel> self();
    ~Mpris2FilterProxyModel() override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &source_parent) const override;

private Q_SLOTS:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);

private:
    explicit Mpris2FilterProxyModel(QObject *parent = nullptr);

    std::shared_ptr<Mpris2SourceModel> m_sourceModel;
    std::unordered_set<unsigned> m_proxyPidList;
};
