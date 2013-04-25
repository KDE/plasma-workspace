/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
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

#ifndef CONFIGUILOADER_H
#define CONFIGUILOADER_H


#include <QQuickView>
#include <QJSValue>
#include <QQmlListProperty>
#include <QStandardItemModel>

namespace Plasma {
    class Applet;
}

class ConfigPropertyMap;


class ConfigCategory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon WRITE setIcon NOTIFY iconChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QString pluginName READ pluginName WRITE setPluginName NOTIFY pluginNameChanged)

public:
    ConfigCategory(QObject *parent = 0);
    ~ConfigCategory();

    QString name() const;
    void setName(const QString &name);

    QString icon() const;
    void setIcon(const QString &icon);

    QString source() const;
    void setSource(const QString &source);

    QString pluginName() const;
    void setPluginName(const QString &pluginName);

Q_SIGNALS:
    void nameChanged();
    void iconChanged();
    void sourceChanged();
    void pluginNameChanged();

private:
    QString m_name;
    QString m_icon;
    QString m_source;
    QString m_pluginName;
};

class ConfigModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<ConfigCategory> categories READ categories CONSTANT)
    Q_CLASSINFO("DefaultProperty", "categories")
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole+1,
        IconRole,
        SourceRole,
        PluginNameRole
    };
    ConfigModel(QObject *parent = 0);
    ~ConfigModel();

    void appendCategory(ConfigCategory *c);
    void clear();

    void setApplet(Plasma::Applet *interface);
    Plasma::Applet *applet() const;

    int count() {return rowCount();}
    virtual int rowCount(const QModelIndex &index = QModelIndex()) const;
    virtual QVariant data(const QModelIndex&, int) const;
    Q_INVOKABLE QVariant get(int row) const;

    QQmlListProperty<ConfigCategory> categories();
    
    static ConfigCategory *categories_at(QQmlListProperty<ConfigCategory> *prop, int index);
    static void categories_append(QQmlListProperty<ConfigCategory> *prop, ConfigCategory *o);
    static int categories_count(QQmlListProperty<ConfigCategory> *prop);
    static void categories_clear(QQmlListProperty<ConfigCategory> *prop);

Q_SIGNALS:
    void countChanged();

private:
    QList<ConfigCategory*>m_categories;
    QWeakPointer<Plasma::Applet> m_appletInterface;
};


//TODO: the config view for the containment should be a subclass
//TODO: is it possible to move this in the shell?
class ConfigView : public QQuickView
{
    Q_OBJECT
    Q_PROPERTY(ConfigModel *configModel READ configModel CONSTANT)

public:
    ConfigView(Plasma::Applet *applet, QWindow *parent = 0);
    virtual ~ConfigView();

    virtual void init();

    ConfigModel *configModel() const;

protected:
     void hideEvent(QHideEvent *ev);
     void resizeEvent(QResizeEvent *re);

private:
    Plasma::Applet *m_applet;
    ConfigModel *m_configModel;
};

#endif // multiple inclusion guard
