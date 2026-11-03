/*
    SPDX-FileCopyrightText: 2011 Lionel Chauvin <megabigbug@yahoo.fr>
    SPDX-FileCopyrightText: 2011, 2012 Cédric Bellegarde <gnumdk@gmail.com>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QDBusObjectPath>
#include <QMenu>

class VerticalMenu : public QMenu
{
    Q_OBJECT
public:
    explicit VerticalMenu(QWidget *parent = nullptr);
    ~VerticalMenu() override;

    QString serviceName() const
    {
        return m_serviceName;
    }
    void setServiceName(const QString &serviceName)
    {
        m_serviceName = serviceName;
    }

    QDBusObjectPath menuObjectPath() const
    {
        return m_menuObjectPath;
    }
    void setMenuObjectPath(const QDBusObjectPath &menuObjectPath)
    {
        m_menuObjectPath = menuObjectPath;
    }

private:
    QString m_serviceName;
    QDBusObjectPath m_menuObjectPath;
};
