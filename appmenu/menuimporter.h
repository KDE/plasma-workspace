/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef MENUIMPORTER_H
#define MENUIMPORTER_H

// Qt
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QWidget> // For WId

class QDBusObjectPath;
class QDBusPendingCallWatcher;
class QDBusServiceWatcher;
class QMenu;

/**
 * Represents an item with its children. GetLayout() returns a
 * DBusMenuLayoutItemList.
 */
struct DBusMenuLayoutItem
{
    int id;
    QVariantMap properties;
    QList<DBusMenuLayoutItem> children;
};
Q_DECLARE_METATYPE(DBusMenuLayoutItem)

class MenuImporter : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    MenuImporter(QObject*);
    ~MenuImporter();

    bool connectToBus();

    bool serviceExist(WId id) { return m_menuServices.contains(id); }
    QString serviceForWindow(WId id) { return m_menuServices.value(id); }

    bool pathExist(WId id) { return m_menuPaths.contains(id); }
    QString pathForWindow(WId id) { return m_menuPaths.value(id).path(); }

    QList<WId> ids() { return m_menuServices.keys(); }

    /**
     * Return id of first transient/friend window with a menu available
     */
    WId recursiveMenuId(WId id);

Q_SIGNALS:
    void WindowRegistered(WId id, const QString& service, const QDBusObjectPath&);
    void WindowUnregistered(WId id);

public Q_SLOTS:
    Q_NOREPLY void RegisterWindow(WId id, const QDBusObjectPath& path);
    Q_NOREPLY void UnregisterWindow(WId id);
    QString GetMenuForWindow(WId id, QDBusObjectPath& path);

private Q_SLOTS:
    void slotServiceUnregistered(const QString& service);
    void slotLayoutUpdated(uint revision, int parentId);
    void finishFakeUnityAboutToShow(QDBusPendingCallWatcher*);

private:
    QDBusServiceWatcher* m_serviceWatcher;
    QHash<WId, QString> m_menuServices;
    QHash<WId, QDBusObjectPath> m_menuPaths;
    QHash<WId, QString> m_windowClasses;

    void fakeUnityAboutToShow();
};

#endif /* MENUIMPORTER_H */
