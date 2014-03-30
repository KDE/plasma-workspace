/*
 *   Copyright 2009 by Chani Armitage <chani@kde.org>
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

#ifndef SWITCHACTIVITY_HEADER
#define SWITCHACTIVITY_HEADER

#include <plasma/containmentactions.h>

class QAction;
class KMenu;

class SwitchActivity : public Plasma::ContainmentActions
{
    Q_OBJECT
    public:
        SwitchActivity(QObject* parent, const QVariantList& args);
        ~SwitchActivity();

        void contextEvent(QEvent *event);
        void contextEvent(QGraphicsSceneMouseEvent *event);
        void wheelEvent(QGraphicsSceneWheelEvent *event);
        QList<QAction*> contextualActions();

    private Q_SLOTS:
        void switchTo(QAction *action);
        void makeMenu();

    private:
        KMenu *m_menu;
        QAction *m_action;
        bool m_useNepomuk;
};

K_EXPORT_PLASMA_CONTAINMENTACTIONS(switchactivity, SwitchActivity)

#endif
