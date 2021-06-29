/*
    SPDX-FileCopyrightText: 2006-2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#pragma once

#include <QSortFilterProxyModel>
#include <thememodel.h>

/**
 * SortProxyModel is a sorting proxy model intended to be used in combination
 * with the ItemDelegate class.
 *
 * First it compares the Qt::DisplayRoles, and if they match it compares
 * the CursorTheme::DisplayDetailRoles.
 *
 * The model assumes both display roles are QStrings.
 */
class SortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit SortProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }
    ~SortProxyModel() override
    {
    }
    QHash<int, QByteArray> roleNames() const override;
    inline const CursorTheme *theme(const QModelIndex &index) const;
    inline QModelIndex findIndex(const QString &name) const;
    inline QModelIndex defaultIndex() const;
    inline void removeTheme(const QModelIndex &index);

private:
    int compare(const QModelIndex &left, const QModelIndex &right, int role) const;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

const CursorTheme *SortProxyModel::theme(const QModelIndex &index) const
{
    CursorThemeModel *model = static_cast<CursorThemeModel *>(sourceModel());
    return model->theme(mapToSource(index));
}

QModelIndex SortProxyModel::findIndex(const QString &name) const
{
    CursorThemeModel *model = static_cast<CursorThemeModel *>(sourceModel());
    return mapFromSource(model->findIndex(name));
}

QModelIndex SortProxyModel::defaultIndex() const
{
    CursorThemeModel *model = static_cast<CursorThemeModel *>(sourceModel());
    return mapFromSource(model->defaultIndex());
}

void SortProxyModel::removeTheme(const QModelIndex &index)
{
    CursorThemeModel *model = static_cast<CursorThemeModel *>(sourceModel());
    model->removeTheme(mapToSource(index));
}
