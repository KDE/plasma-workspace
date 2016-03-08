/*
 *   Copyright 2016 Marco Martin <mart@kde.org>
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

#ifndef ROOTITEM_H
#define ROOTITEM_H

#include <QQuickItem>

#include <Plasma/Applet>

namespace Plasma
{
class Applet;
}


class RootItem : public QQuickItem
{
    Q_OBJECT

    Q_PROPERTY(QQuickItem *compactRepresentationItem READ compactRepresentationItem NOTIFY compactRepresentationItemChanged)

    Q_PROPERTY(QQuickItem *fullRepresentationItem READ fullRepresentationItem NOTIFY fullRepresentationItemChanged)

    Q_PROPERTY(bool expanded READ isExpanded CONSTANT)

    Q_PROPERTY(QObject *rootItem READ rootItem NOTIFY rootItemChanged)

public:
    RootItem(QQuickItem *parent = 0);
    ~RootItem();

    void setApplet(Plasma::Applet *applet);
    Plasma::Applet *applet();

////PROPERTY ACCESSORS

    QQuickItem *compactRepresentationItem();

    QQuickItem *fullRepresentationItem();

    bool isExpanded() const;

Q_SIGNALS:
    void compactRepresentationItemChanged();
    void fullRepresentationItemChanged();

private:
    QPointer <Plasma::Applet> m_applet;
};

}

#endif
