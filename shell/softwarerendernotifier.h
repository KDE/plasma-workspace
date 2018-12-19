/*
 *  Copyright 2018 David Edmundson <davidedmundson@kde.org>
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

#ifndef SOFTWARERENDERNOTIFIER_H
#define SOFTWARERENDERNOTIFIER_H

#include <KStatusNotifierItem>

/**
 * Responsible for showing an SNI if the software renderer is used
 * to allow the a user to open the KCM
 */

class SoftwareRendererNotifier: public KStatusNotifierItem
{
    Q_OBJECT
public:
    //only exposed as void static constructor as internally it is self memory managing
    static void notifyIfRelevant();
private:
    SoftwareRendererNotifier(QObject *parent=nullptr);
    ~SoftwareRendererNotifier();
};

#endif
