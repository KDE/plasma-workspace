/*
    localegeneratorbase.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "localegeneratorbase.h"

void LocaleGeneratorBase::localesGenerate(const QStringList &list)
{
    Q_UNUSED(list)
    Q_EMIT userHasToGenerateManually(defaultManuallyGenerateMessage());
}
