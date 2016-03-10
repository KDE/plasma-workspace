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

#include <kactivities/consumer.h>
#include <kactivities/controller.h>

class QAction;

class SwitchActivity : public Plasma::ContainmentActions
{
    Q_OBJECT
    public:
        SwitchActivity(QObject* parent, const QVariantList& args);
        ~SwitchActivity() override;

        QList<QAction*> contextualActions() override;

        void performNextAction() override;
        void performPreviousAction() override;

    private Q_SLOTS:
        void switchTo(QAction *action);
        void makeMenu();

    private:
        QList<QAction *>m_actions;
        KActivities::Consumer m_consumer;
        KActivities::Controller m_controller;
};

#endif
