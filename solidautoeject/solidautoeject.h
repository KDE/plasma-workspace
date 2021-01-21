/* This file is part of the KDE Project
   Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef SOLIDAUTOEJECT_H
#define SOLIDAUTOEJECT_H

#include <kdedmodule.h>

namespace Solid
{
class Device;
}

class SolidAutoEject : public KDEDModule
{
    Q_OBJECT

public:
    SolidAutoEject(QObject *parent, const QList<QVariant> &);
    ~SolidAutoEject() override;

private slots:
    void onDeviceAdded(const QString &udi);
    void onEjectPressed(const QString &udi);

private:
    void connectDevice(const Solid::Device &device);
};

#endif
