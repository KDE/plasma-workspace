/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
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

#include "monitoricon.h"
#include <QDebug>
#include <QPainter>
#include <KIcon>
#include <KIconLoader>

#define MARGIN 5

class MonitorIcon::Private
{
    public:
        Private() : imageSize(22, 22) { }

        QSizeF imageSize;
        QString image;
        QStringList overlays;
};

MonitorIcon::MonitorIcon(QGraphicsItem *parent) :
        QGraphicsWidget(parent),
        d(new Private)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setPreferredSize(d->imageSize.width() + 2 * MARGIN, d->imageSize.height() + 2 * MARGIN);
}

MonitorIcon::~MonitorIcon()
{
    delete d;
}

QString MonitorIcon::image() const
{
    return d->image;
}

void MonitorIcon::setImage(const QString &image)
{
    d->image = image;
    update();
}

QStringList MonitorIcon::overlays() const
{
    return d->overlays;
}

void MonitorIcon::setOverlays( const QStringList & overlays )
{
    d->overlays = overlays;
    update();
}

void MonitorIcon::paint(QPainter *p,
                        const QStyleOptionGraphicsItem *option,
                        QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    p->drawPixmap(QPointF((size().width() - d->imageSize.width()) / 2,
                          (size().height() - d->imageSize.height()) / 2),
                  KIcon(d->image, KIconLoader::global(),
                        d->overlays).pixmap(d->imageSize.toSize()));
}

#include "monitoricon.moc"
