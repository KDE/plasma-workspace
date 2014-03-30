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

#ifndef MONITORICON_HEADER
#define MONITORICON_HEADER

#include <QGraphicsWidget>
#include "sm_export.h"

class SM_EXPORT MonitorIcon : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY(QString image READ image WRITE setImage)

public:
    explicit MonitorIcon(QGraphicsItem *parent = 0);
    virtual ~MonitorIcon();

    QString image() const;
    void setImage(const QString &image);

    QStringList overlays() const;
    void setOverlays( const QStringList & overlays );
protected:
    virtual void paint(QPainter *p,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget = 0);
private:
    class Private;
    Private * const d;
};

#endif
