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

#include "monitorbutton.h"

#include <QIcon>
#include <QPainter>
#include <QTimeLine>

#include <QDebug>
#include <KIcon>
#include <KPushButton>

#include <Plasma/PaintUtils>

#define MARGIN 2

class MonitorButton::Private
{
public:
    Private() : imageSize(32, 32)
    {
    }

    QSize imageSize;
    QString image;
    KIcon icon;
    QTimeLine highlighter;
};

MonitorButton::MonitorButton(QGraphicsWidget *parent) :
        Plasma::PushButton(parent),
        d(new Private)
{
   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
   setPreferredSize(d->imageSize.width() + 2 * MARGIN, d->imageSize.height() + 2 * MARGIN);

   d->highlighter.setDuration(100);
   d->highlighter.setFrameRange(0, 10);
   d->highlighter.setCurveShape(QTimeLine::EaseInCurve);
   connect(&d->highlighter, SIGNAL(valueChanged(qreal)), this, SLOT(highlight()));
}

MonitorButton::~MonitorButton()
{
    delete d;
}

QString MonitorButton::image() const
{
    return d->image;
}

void MonitorButton::setImage(const QString &image)
{
    d->image = image;
    d->icon = KIcon(image);
    update();
}

void MonitorButton::highlight()
{
    update();
}

void MonitorButton::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED(event)

    d->highlighter.setDirection(QTimeLine::Forward);
    if (d->highlighter.currentValue() < 1 &&
        d->highlighter.state() == QTimeLine::NotRunning) {
        d->highlighter.start();
    }
}

void MonitorButton::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED(event)

    d->highlighter.setDirection(QTimeLine::Backward);

    if (d->highlighter.currentValue() > 0 &&
        d->highlighter.state() == QTimeLine::NotRunning) {
        d->highlighter.start();
    }
}

void MonitorButton::paint(QPainter *p,
                          const QStyleOptionGraphicsItem *option,
                          QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QIcon::Mode mode = QIcon::Disabled;
    if (isChecked()) {
        mode = QIcon::Normal;
    }

    QPixmap icon = Plasma::PaintUtils::transition(d->icon.pixmap(d->imageSize, QIcon::Disabled),
                                                  d->icon.pixmap(d->imageSize, QIcon::Normal),
                                                  isChecked() ? 1 : d->highlighter.currentValue());
    p->drawPixmap(QPointF((size().width() - d->imageSize.width()) / 2,
                        (size().height() - d->imageSize.height()) / 2),
                  icon);
}

#include "monitorbutton.moc"
