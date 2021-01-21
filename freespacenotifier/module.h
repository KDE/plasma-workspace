/* This file is part of the KDE Project
   Copyright (c) 2006 Lukas Tinkl <ltinkl@suse.cz>
   Copyright (c) 2008 Lubos Lunak <l.lunak@suse.cz>
   Copyright (c) 2009 Ivo Anjo <knuckles@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MODULE_H
#define MODULE_H

#include <KDEDModule>
#include <QObject>

#include "freespacenotifier.h"

class FreeSpaceNotifierModule : public KDEDModule
{
    Q_OBJECT
public:
    FreeSpaceNotifierModule(QObject *parent, const QList<QVariant> &);

private:
    void showConfiguration();
};

#endif
