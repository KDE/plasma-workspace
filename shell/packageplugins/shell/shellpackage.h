/*
    SPDX-FileCopyrightText: 2013 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KPackage/PackageStructure>

class ShellPackage : public KPackage::PackageStructure
{
public:
    ShellPackage(QObject *parent, const QVariantList &list);
    void initPackage(KPackage::Package *package) override;
    void pathChanged(KPackage::Package *package) override;
};
