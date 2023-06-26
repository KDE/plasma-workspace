/*
    SPDX-FileCopyrightText: 2014 John Layt <john@layt.net>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "config-workspace.h"

#include "localegenerator.h"

#include "localegeneratorbase.h"

#include "localegeneratorgeneratedglibc.h"

#if UBUNTU_LOCALE
#include "localegeneratorubuntu.h"
#elif GLIBC_LOCALE_AUTO
#include "localegeneratorglibc.h"
#endif

LocaleGeneratorBase *LocaleGenerator::getGenerator()
{
#if UBUNTU_LOCALE
    static LocaleGeneratorUbuntu singleton;
#elif GLIBC_LOCALE_AUTO
    static LocaleGeneratorGlibc singleton;
#elif GLIBC_LOCALE_GENERATED
    static LocaleGeneratorGeneratedGlibc singleton;
#else
    static LocaleGeneratorBase singleton;
#endif
    return &singleton;
}
