/*
    Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

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
#include <QQmlParserStatus>

#include <QPixmap>
#include <QSize>
#include <QUrl>


class Thumbnailer : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QSize size READ size WRITE setSize NOTIFY sizeChanged)

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool hasPreview READ hasPreview NOTIFY pixmapChanged)
    Q_PROPERTY(QPixmap pixmap READ pixmap NOTIFY pixmapChanged)
    Q_PROPERTY(QSize pixmapSize READ pixmapSize NOTIFY pixmapChanged)

    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)

    Q_PROPERTY(bool menuVisible READ menuVisible NOTIFY menuVisibleChanged)

public:
    explicit Thumbnailer(QObject *parent = nullptr);
    ~Thumbnailer() override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QSize size() const;
    void setSize(const QSize &size);

    bool busy() const;
    bool hasPreview() const;
    QPixmap pixmap() const;
    QSize pixmapSize() const;

    QString iconName() const;

    bool menuVisible() const;

    void classBegin() override;
    void componentComplete() override;

signals:
    void menuVisibleChanged();

    void urlChanged();
    void sizeChanged();
    void busyChanged();
    void pixmapChanged();
    void iconNameChanged();

private:
    void generatePreview();

    bool m_inited = false;

    bool m_menuVisible = false;

    QUrl m_url;
    QSize m_size;

    bool m_busy = false;

    QPixmap m_pixmap;

    QString m_iconName;

};
