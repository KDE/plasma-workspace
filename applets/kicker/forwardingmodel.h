/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"

#include <QPointer>

class ForwardingModel : public AbstractModel
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)

public:
    explicit ForwardingModel(QObject *parent = nullptr);
    ~ForwardingModel() override;

    QString description() const override;

    QAbstractItemModel *sourceModel() const;
    virtual void setSourceModel(QAbstractItemModel *sourceModel);

    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

    Q_INVOKABLE QString labelForRow(int row) override;

    Q_INVOKABLE AbstractModel *modelForRow(int row) override;

    AbstractModel *favoritesModel() override;

    int separatorCount() const override;

public Q_SLOTS:
    void reset();

Q_SIGNALS:
    void sourceModelChanged() const;

protected:
    QModelIndex indexToSourceIndex(const QModelIndex &index) const;

    void connectSignals();
    void disconnectSignals();

    QPointer<QAbstractItemModel> m_sourceModel;
};
