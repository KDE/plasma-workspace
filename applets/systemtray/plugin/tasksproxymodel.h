/*
 * Copyright (C) 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef TASKSPROXYMODEL_H
#define TASKSPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace SystemTray
{

class Host;
class Task;

class TasksProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_ENUMS(Category)

    Q_PROPERTY(SystemTray::Host *host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(Category category READ category WRITE setCategory NOTIFY categoryChanged)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    TasksProxyModel(QObject *parent = nullptr);
    virtual ~TasksProxyModel() = default;

    enum class Category {
        NoTasksCategory,
        HiddenTasksCategory,
        ShownTasksCategory
    };

    Host *host() const;
    void setHost(Host *host);

    Category category() const;
    void setCategory(Category category);

    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

signals:
    void hostChanged();
    void categoryChanged();
    void countChanged();

private:
    bool showTask(Task *task) const;

    Host *m_host = nullptr;
    Category m_category = Category::NoTasksCategory;

};

} // namespace SystemTray

#endif // TASKSPROXYMODEL_H
