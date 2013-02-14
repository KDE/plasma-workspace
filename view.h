/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VIEW_H
#define VIEW_H

#include <QtQuick/QQuickView>


#include "plasma/corona.h"
#include "plasma/containment.h"


class View : public QQuickView
{
    Q_OBJECT

public:
    explicit View(Plasma::Corona *corona, QWindow *parent = 0);
    virtual ~View();

    //FIXME: not super nice, but we have to be sure qml assignment is done after window flags
    void init();

    void setContainment(Plasma::Containment *cont);
    Plasma::Containment *containment() const;

private:
    Plasma::Corona *m_corona;
    QWeakPointer<Plasma::Containment> m_containment;
};

#endif // VIEW_H
