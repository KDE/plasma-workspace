/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

// Qt
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QDebug>
#include <QObject>
#include <qwindowdefs.h>

class AppmenuDBus : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit AppmenuDBus(QObject *);
    ~AppmenuDBus() override;

    bool connectToBus(const QString &service = QString(), const QString &path = QString());

    /**
     * DBus method showing menu at QPoint(x,y) for given DBus service name and menuObjectPath
     * if x or y == -1, show in application window
     */
    void showMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * DBus method reconfiguring kded module
     */
    void reconfigure();

Q_SIGNALS:
    /**
     * This signal is emitted on showMenu() request
     */
    void appShowMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * This signal is emitted on reconfigure() request
     */
    void reconfigured();

    // Dbus signals
    /**
     * This signal is emitted whenever kded want to show menu
     * We do not know where is menu decoration button, so tell kwin to show menu
     */
    void showRequest(const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * This signal is emitted whenever popup menu/menubar is shown
     * Useful for decorations to know if menu button should look pressed
     */
    void menuShown(const QString &serviceName, const QDBusObjectPath &menuObjectPath);
    /**
     * This signal is emitted whenever popup menu/menubar is hidden
     * Useful for decorations to know if menu button should be release
     */
    void menuHidden(const QString &serviceName, const QDBusObjectPath &menuObjectPath);

private:
    QString m_service;
};
