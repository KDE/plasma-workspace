/*
    localegeneratorbase.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "localegeneratorbase.h"

#include <KLocalizedString>

void LocaleGeneratorBase::localesGenerate(const QStringList &list)
{
    Q_UNUSED(list)
    Q_EMIT userHasToGenerateManually(i18nc("@info:warning",
                                           "Locale has been configured, but this KCM currently "
                                           "doesn't support auto locale generation on non-glibc systems, "
                                           "please refer to your distribution's manual to install fonts and generate locales"));
}
