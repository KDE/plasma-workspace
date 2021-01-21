/*
    Copyright (C) 2016,2019 Kai Uwe Broulik <kde@privat.broulik.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

class QAction;
class QQuickItem;

class FileMenu : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QQuickItem *visualParent READ visualParent WRITE setVisualParent NOTIFY visualParentChanged)
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged)

public:
    explicit FileMenu(QObject *parent = nullptr);
    ~FileMenu() override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QQuickItem *visualParent() const;
    void setVisualParent(QQuickItem *visualParent);

    bool visible() const;
    void setVisible(bool visible);

    Q_INVOKABLE void open(int x, int y);

signals:
    void actionTriggered(QAction *action);

    void urlChanged();
    void visualParentChanged();
    void visibleChanged();

private:
    QUrl m_url;
    QPointer<QQuickItem> m_visualParent;
    bool m_visible = false;
};
