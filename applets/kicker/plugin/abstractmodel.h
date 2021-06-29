/*
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>

class AbstractEntry;

class AbstractModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int separatorCount READ separatorCount NOTIFY separatorCountChanged)
    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize NOTIFY iconSizeChanged)
    Q_PROPERTY(AbstractModel *favoritesModel READ favoritesModel WRITE setFavoritesModel NOTIFY favoritesModelChanged)

public:
    explicit AbstractModel(QObject *parent = nullptr);
    ~AbstractModel() override;

    QHash<int, QByteArray> roleNames() const override;

    virtual QString description() const = 0;

    int count() const;
    virtual int separatorCount() const;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    int iconSize() const;
    void setIconSize(int size);

    Q_INVOKABLE virtual bool trigger(int row, const QString &actionId, const QVariant &argument) = 0;

    Q_INVOKABLE virtual void refresh();

    Q_INVOKABLE virtual QString labelForRow(int row);

    Q_INVOKABLE virtual AbstractModel *modelForRow(int row);
    Q_INVOKABLE virtual int rowForModel(AbstractModel *model);

    virtual bool hasActions() const;
    virtual QVariantList actions() const;

    virtual AbstractModel *favoritesModel();
    virtual void setFavoritesModel(AbstractModel *model);
    AbstractModel *rootModel();

    virtual void entryChanged(AbstractEntry *entry);

Q_SIGNALS:
    void descriptionChanged() const;
    void countChanged() const;
    void separatorCountChanged() const;
    void iconSizeChanged() const;
    void favoritesModelChanged() const;

protected:
    AbstractModel *m_favoritesModel;

private:
    int m_iconSize;
};
