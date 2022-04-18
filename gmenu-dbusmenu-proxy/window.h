/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusContext>
#include <QMultiHash>
#include <QObject>
#include <QString>
#include <QVector>
#include <QWindow> // for WId

#include <functional>

#include "../libdbusmenuqt/dbusmenutypes_p.h"
#include "gdbusmenutypes_p.h"

class QDBusVariant;

class Actions;
class Menu;

class Window : public QObject, protected QDBusContext
{
    Q_OBJECT

    // DBus
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(uint Version READ version)

public:
    Window(const QString &serviceName);
    ~Window() override;

    void init();

    WId winId() const;
    void setWinId(WId winId);

    QString serviceName() const;

    QString applicationObjectPath() const;
    void setApplicationObjectPath(const QString &applicationObjectPath);

    QString unityObjectPath() const;
    void setUnityObjectPath(const QString &unityObjectPath);

    QString windowObjectPath() const;
    void setWindowObjectPath(const QString &windowObjectPath);

    QString applicationMenuObjectPath() const;
    void setApplicationMenuObjectPath(const QString &applicationMenuObjectPath);

    QString menuBarObjectPath() const;
    void setMenuBarObjectPath(const QString &menuBarObjectPath);

    QString currentMenuObjectPath() const;

    QString proxyObjectPath() const;

    // DBus
    bool AboutToShow(int id);
    void Event(int id, const QString &eventId, const QDBusVariant &data, uint timestamp);
    DBusMenuItemList GetGroupProperties(const QList<int> &ids, const QStringList &propertyNames);
    uint GetLayout(int parentId, int recursionDepth, const QStringList &propertyNames, DBusMenuLayoutItem &dbusItem);
    QDBusVariant GetProperty(int id, const QString &property);

    QString status() const;
    uint version() const;

Q_SIGNALS:
    // don't want to pollute X stuff into Menu, let all of that be in MenuProxy
    void requestWriteWindowProperties();
    void requestRemoveWindowProperties();

    // DBus
    void ItemActivationRequested(int id, uint timestamp);
    void ItemsPropertiesUpdated(const DBusMenuItemList &updatedProps, const DBusMenuItemKeysList &removedProps);
    void LayoutUpdated(uint revision, int parent);

private:
    void initMenu();

    bool registerDBusObject();
    void updateWindowProperties();

    bool getAction(const QString &name, GMenuAction &action) const;
    void triggerAction(const QString &name, const QVariant &target, uint timestamp = 0);
    Actions *getActionsForAction(const QString &name, QString &lookupName) const;

    void menuChanged(const QVector<uint> &menuIds);
    void menuItemsChanged(const QVector<uint> &itemIds);

    void onActionsChanged(const QStringList &dirty, const QString &prefix);
    void onMenuSubscribed(uint id);

    QVariantMap gMenuToDBusMenuProperties(const QVariantMap &source) const;

    WId m_winId = 0;
    QString m_serviceName; // original GMenu service (the gtk app)

    QString m_applicationObjectPath;
    QString m_unityObjectPath;
    QString m_windowObjectPath;
    QString m_applicationMenuObjectPath;
    QString m_menuBarObjectPath;

    QString m_currentMenuObjectPath;

    QString m_proxyObjectPath; // our object path on this proxy app

    QMultiHash<int, QDBusMessage> m_pendingGetLayouts;

    Menu *m_applicationMenu = nullptr;
    Menu *m_menuBar = nullptr;

    Menu *m_currentMenu = nullptr;

    Actions *m_applicationActions = nullptr;
    Actions *m_unityActions = nullptr;
    Actions *m_windowActions = nullptr;

    bool m_menuInited = false;
};
