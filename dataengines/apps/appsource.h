/*
 * Copyright 2009 Chani Armitage <chani@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef APPSOURCE_H
#define APPSOURCE_H

// plasma
#include <Plasma/DataContainer>

#include <KService>
#include <KServiceGroup>

/**
 * App Source
 */
class AppSource : public Plasma::DataContainer
{

    Q_OBJECT

    public:
        AppSource(KServiceGroup::Ptr startup, QObject *parent);
        AppSource(KService::Ptr app, QObject *parent);
        ~AppSource() override;

    protected:
        Plasma::Service *createService();
        KService::Ptr getApp();
        bool isApp() const;

    private Q_SLOTS:
        void updateGroup();
        void updateApp();

    private:
        friend class AppsEngine;
        friend class AppJob;
        KServiceGroup::Ptr m_group;
        KService::Ptr m_app;
        bool m_isApp;

};

#endif // APPSOURCE_H
