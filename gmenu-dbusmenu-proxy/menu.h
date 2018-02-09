/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
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

#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QWindow> // for WId

#include "gdbusmenutypes_p.h"
#include "../libdbusmenuqt/dbusmenutypes_p.h"

class QDBusVariant;

class Menu : public QObject
{
    Q_OBJECT

    // DBus
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(uint Version READ version)

public:
    Menu(WId winId, const QString &serviceName, const QString &objectPath);
    ~Menu();

    WId winId() const;
    QString serviceName() const;
    QString objectPath() const;
    QString proxyObjectPath() const;

    // DBus
    bool AboutToShow(int id);
    void Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp);
    DBusMenuItemList GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames);
    uint GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, DBusMenuLayoutItem &item);
    QDBusVariant GetProperty(int id, const QString &property);

    QString status() const;
    uint version() const;

signals:
    // don't want to pollute X stuff into Menu, let all of that be in MenuProxy
    void requestWriteWindowProperties();
    void requestRemoveWindowProperties();

    // DBus
    void ItemActivationRequested(int id, uint timestamp);
    void ItemsPropertiesUpdated(const DBusMenuItemList &updatedProps, const DBusMenuItemKeysList &removedProps);
    void LayoutUpdated(uint revision, int parent);

private slots:
    void onMenuChanged(const GMenuChangeList &changes);

private:
    void start(const QList<uint> &ids);
    void stop(const QList<uint> &ids);

    bool registerDBusObject();

    WId m_winId;
    QString m_serviceName; // original GMenu service (the gtk app)
    QString m_objectPath; // original GMenu object path (the gtk app)

    QString m_proxyObjectPath; // our object path on this proxy app

    QList<uint> m_subscriptions; // keeps track of which menu trees we're subscribed to

    QHash<uint, VariantHashList> m_menus; // the menu data we gathered in Start + subsequent updates

};
