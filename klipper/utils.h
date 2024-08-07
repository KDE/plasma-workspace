/*
    SPDX-FileCopyrightText: 2022 Popov Eugene <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

#include "klipper_export.h"

class KLIPPER_EXPORT Utils
{
public:
    /**
     * Returns a simplified text of a maximum length of maxLength
     */
    static QString simplifiedText(const QString &text, int maxLength);
};
