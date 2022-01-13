/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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

private Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onEjectPressed(const QString &udi);

private:
    void connectDevice(const Solid::Device &device);
};
