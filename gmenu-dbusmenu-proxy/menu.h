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
#include <QDBusContext>
#include <QString>
#include <QVector>
#include <QWindow> // for WId

#include <functional>

#include "gdbusmenutypes_p.h"
#include "../libdbusmenuqt/dbusmenutypes_p.h"

class QDBusVariant;

class Menu : public QObject, protected QDBusContext
{
    Q_OBJECT

    // DBus
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(uint Version READ version)

public:
    Menu(WId winId,
         const QString &serviceName,
         const QString &applicationObjectPath,
         const QString &windowObjectPath,
         const QString &menuObjectPath);
    ~Menu();

    void cleanup();

    WId winId() const;
    QString serviceName() const;

    QString applicationObjectPath() const;
    QString windowObjectPath() const;
    QString menuObjectPath() const;

    QString proxyObjectPath() const;

    // DBus
    bool AboutToShow(int id);
    void Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp);
    DBusMenuItemList GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames);
    uint GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, DBusMenuLayoutItem &dbusItem);
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
    void onApplicationActionsChanged(const GMenuActionsChange &changes);
    void onWindowActionsChanged(const GMenuActionsChange &changes);

private:
    void init();
    void start(uint id);
    void stop(const QList<uint> &id);

    bool registerDBusObject();

    void getActions(const QString &path, const std::function<void(GMenuActionMap,bool)> &cb);
    bool getAction(const QString &name, GMenuAction &action) const;
    void triggerAction(const QString &name, uint timestamp = 0);

    static int treeStructureToInt(int subscription, int section, int index);
    static void intToTreeStructure(int source, int &subscription, int &section, int &index);

    static GMenuItem findSection(const QList<GMenuItem> &list, int section);

    QVariantMap gMenuToDBusMenuProperties(const QVariantMap &source) const;

    WId m_winId;
    QString m_serviceName; // original GMenu service (the gtk app)

    QString m_applicationObjectPath;
    QString m_windowObjectPath;
    QString m_menuObjectPath;

    QString m_proxyObjectPath; // our object path on this proxy app

    // QSet?
    QList<uint> m_subscriptions; // keeps track of which menu trees we're subscribed to

    QHash<uint, GMenuItemList> m_menus;

    QHash<int, QDBusMessage> m_pendingGetLayouts;

    bool m_queriedApplicationActions = false;
    GMenuActionMap m_applicationActions;
    bool m_queriedWindowActions = false;
    GMenuActionMap m_windowActions;

};
