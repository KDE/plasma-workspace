/*
    localegeneratorgeneratedglibc.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once
#include "localegeneratorbase.h"

class LocaleGeneratorGeneratedGlibc : public LocaleGeneratorBase
{
    Q_OBJECT
public:
    using LocaleGeneratorBase::LocaleGeneratorBase;
    void localesGenerate(const QStringList &list) override;
};
