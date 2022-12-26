/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#pragma once

// Qt
#include <QDBusArgument>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>
#include <QWidget> // For WId

class QDBusServiceWatcher;

class MenuImporter : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit MenuImporter(QObject *);
    ~MenuImporter() override;

    bool connectToBus();

    bool serviceExist(WId id)
    {
        return m_menuServices.contains(id);
    }
    QString serviceForWindow(WId id)
    {
        return m_menuServices.value(id);
    }

    bool pathExist(WId id)
    {
        return m_menuPaths.contains(id);
    }
    QString pathForWindow(WId id)
    {
        return m_menuPaths.value(id).path();
    }

    QList<WId> ids()
    {
        return m_menuServices.keys();
    }

Q_SIGNALS:
    void WindowRegistered(WId id, const QString &service, const QDBusObjectPath &);
    void WindowUnregistered(WId id);

public Q_SLOTS:
    Q_NOREPLY void RegisterWindow(WId id, const QDBusObjectPath &path);
    Q_NOREPLY void UnregisterWindow(WId id);
    QString GetMenuForWindow(WId id, QDBusObjectPath &path);

private Q_SLOTS:
    void slotServiceUnregistered(const QString &service);

private:
    QDBusServiceWatcher *m_serviceWatcher;
    QHash<WId, QString> m_menuServices;
    QHash<WId, QDBusObjectPath> m_menuPaths;
    QHash<WId, QString> m_windowClasses;
};
