/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_KCATEGORIZEDITEMSVIEWMODELS_P_H
#define PLASMA_KCATEGORIZEDITEMSVIEWMODELS_P_H

#include <QIcon>
#include <QPair>
#include <QSortFilterProxyModel>
#include <QStandardItem>

namespace KCategorizedItemsViewModels
{
typedef QPair<QString, QVariant> Filter;

/**
 * Abstract class that needs to be implemented and used with the ItemModel
 */
class AbstractItem : public QStandardItem
{
public:
    /**
     * Returns a localized string - name of the item
     */
    virtual QString name() const;

    /**
     * Returns a unique id related to this item
     */
    virtual QString id() const;

    /**
     * Returns a localized string - description of the item
     */
    virtual QString description() const;

    /**
     * Returns if the item is flagged as favorite
     * Default implementation checks if the item passes the Filter("favorite", "1") filter
     */
    virtual bool isFavorite() const;

    /**
     * Returns the item's number of running applets
     * Default implementation just returns 0
     */
    virtual int running() const;

    /**
     * Returns if the item contains string specified by pattern.
     * Default implementation checks whether name or description contain the
     * string (not needed to be exactly that string)
     */
    virtual bool matches(const QString &pattern) const;

    /**
     * sets the number of running applets for the item
     */
    virtual void setRunning(int count) = 0;

    /**
     * Returns if the item passes the filter specified
     */
    virtual bool passesFiltering(const Filter &filter) const = 0;

private:
};

/**
 * The default implementation of the model containing filters
 */
class DefaultFilterModel : public QStandardItemModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        FilterTypeRole = Qt::UserRole + 1,
        FilterDataRole = Qt::UserRole + 2,
        SeparatorRole = Qt::UserRole + 3,
    };
    explicit DefaultFilterModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    /**
     * Adds a filter to the model
     * @param caption The localized string to be displayed as a name of the filter
     * @param filter The filter structure
     * @param icon The filter icon
     */
    void addFilter(const QString &caption, const Filter &filter, const QIcon &icon = QIcon());

    /**
     * Adds a separator to the model
     * @param caption The localized string to be displayed as a name of the separator
     */
    void addSeparator(const QString &caption);

    int count()
    {
        return rowCount(QModelIndex());
    }

    Q_INVOKABLE QVariantHash get(int i) const;

Q_SIGNALS:
    void countChanged();
};

/**
 * Default filter proxy model.
 */
class DefaultItemFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterChanged)
    Q_PROPERTY(QVariant filterQuery READ filterQuery WRITE setFilterQuery NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit DefaultItemFilterProxyModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    void setSearchTerm(const QString &pattern);
    QString searchTerm() const;

    void setFilterType(const QString type);
    QString filterType() const;

    void setFilterQuery(const QVariant query);
    QVariant filterQuery() const;

    void setFilter(const Filter &filter);

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    QAbstractItemModel *sourceModel() const;

    int columnCount(const QModelIndex &index) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int count()
    {
        return rowCount(QModelIndex());
    }

    Q_INVOKABLE QVariantHash get(int i) const;

Q_SIGNALS:
    void searchTermChanged(const QString &term);
    void filterChanged();
    void countChanged();

private:
    Filter m_filter;
    QString m_searchPattern;
};

} // end of namespace

Q_DECLARE_METATYPE(KCategorizedItemsViewModels::Filter)

#endif /*KCATEGORIZEDITEMSVIEWMODELS_H_*/
