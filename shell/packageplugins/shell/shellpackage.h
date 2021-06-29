/*
    SPDX-FileCopyrightText: 2013 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SHELLPACKAGE_PLUGIN_H
#define SHELLPACKAGE_PLUGIN_H

#include <KPackage/PackageStructure>

class ShellPackage : public KPackage::PackageStructure
{
public:
    ShellPackage(QObject *parent, const QVariantList &list);
    void initPackage(KPackage::Package *package) override;
    void pathChanged(KPackage::Package *package) override;
};

#endif // SHELLPACKAGE_PLUGIN_H
