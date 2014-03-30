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

#ifndef MONITORBUTTON_HEADER
#define MONITORBUTTON_HEADER

#include <Plasma/PushButton>
#include "sm_export.h"

class SM_EXPORT MonitorButton : public Plasma::PushButton
{
    Q_OBJECT
    Q_PROPERTY(QString image READ image WRITE setImage)

public:
    explicit MonitorButton(QGraphicsWidget *parent = 0);
    virtual ~MonitorButton();

    QString image() const;
    void setImage(const QString &image);

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent * event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);
    virtual void paint(QPainter *p,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget = 0);

private Q_SLOTS:
    void highlight();

private:
    class Private;
    Private * const d;
};

#endif
