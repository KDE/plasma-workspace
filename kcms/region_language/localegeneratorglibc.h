/*
    localegeneratorglibc.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "localegeneratorbase.h"
#include "localegenhelperinterface.h"

using LocaleGenHelper = org::kde::localegenhelper::LocaleGenHelper;

class LocaleGeneratorGlibc : public LocaleGeneratorBase
{
    Q_OBJECT
public:
    explicit LocaleGeneratorGlibc(QObject *parent = nullptr);
    Q_INVOKABLE void localesGenerate(const QStringList &list) override;

private:
    LocaleGenHelper *m_interface = nullptr;
};
