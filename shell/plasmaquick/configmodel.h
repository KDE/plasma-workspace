/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
 *   Copyright 2015 Eike Hein <hein@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CONFIGMODEL_H
#define CONFIGMODEL_H

#include <QQmlListProperty>
#include <QAbstractListModel>


//
//  W A R N I N G
//  -------------
//
// This file is not part of the public Plasma API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

namespace Plasma
{
class Applet;
}

namespace PlasmaQuick
{

class ConfigPropertyMap;

class ConfigCategoryPrivate;

class ConfigModelPrivate;
class ConfigCategory;

/*
 * This model contains all the possible config categories for a dialog,
 * such as categories of the config dialog for an Applet
 * TODO: it should probably become an import instead of a library?
 */
class ConfigModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<PlasmaQuick::ConfigCategory> categories READ categories CONSTANT)
    Q_CLASSINFO("DefaultProperty", "categories")
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        SourceRole,
        PluginNameRole,
        VisibleRole
    };
    ConfigModel(QObject *parent = 0);
    ~ConfigModel() override;

    /**
     * add a new category in the model
     * @param ConfigCategory the new category
     **/
    void appendCategory(const QString &iconName, const QString &name,
                        const QString &path, const QString &pluginName);

    void appendCategory(const QString &iconName, const QString &name,
                        const QString &path, const QString &pluginName, bool visible);

    /**
     * clears the model
     **/
    void clear();

    void setApplet(Plasma::Applet *interface);
    Plasma::Applet *applet() const;

    int count()
    {
        return rowCount();
    }
    int rowCount(const QModelIndex &index = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &, int) const Q_DECL_OVERRIDE;

    /**
     * @param row the row for which the data will be returned
     * @raturn the data of the specified row
     **/
    Q_INVOKABLE QVariant get(int row) const;

    /**
     * @return the categories of the model
     **/
    QQmlListProperty<ConfigCategory> categories();

Q_SIGNALS:
    /**
     * emitted when the count is changed
     **/
    void countChanged();

private:
    friend class ConfigModelPrivate;
    ConfigModelPrivate *const d;
};

}

#endif // multiple inclusion guard
