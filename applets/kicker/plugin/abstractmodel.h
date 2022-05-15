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

    /**
     * @return all sections in the model
     */
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)

public:
    explicit AbstractModel(QObject *parent = nullptr);
    ~AbstractModel() override;

    QHash<int, QByteArray> roleNames() const override;

    virtual QString description() const = 0;

    int count() const;
    virtual int separatorCount() const;

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

    virtual QVariantList sections() const;

    virtual void entryChanged(AbstractEntry *entry);

Q_SIGNALS:
    void descriptionChanged() const;
    void countChanged() const;
    void separatorCountChanged() const;
    void iconSizeChanged() const;
    void favoritesModelChanged() const;
    void sectionsChanged() const;

protected:
    AbstractModel *m_favoritesModel = nullptr;

private:
    int m_iconSize = 32;
};
