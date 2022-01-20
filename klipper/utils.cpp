/*
    SPDX-FileCopyrightText: 2022 Popov Eugene <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "utils.h"

QString Utils::simplifiedText(const QString &text, int maxLength)
{
    if (text.length() <= maxLength) {
        return text.simplified();
    }

    QString simplifiedText;
    simplifiedText.reserve(maxLength);

    bool wasSpaceBefore = false;
    for (int i = 0, n = text.length(); i < n; ++i) {
        if (simplifiedText.length() == maxLength) {
            break;
        }
        const QChar c = text.at(i);
        if (c.isSpace()) {
            if (wasSpaceBefore || simplifiedText.isEmpty()) {
                continue;
            }
            simplifiedText.append(QLatin1Char(' '));
            wasSpaceBefore = true;
        } else {
            simplifiedText.append(c);
            wasSpaceBefore = false;
        }
    }

    if (simplifiedText.endsWith(QLatin1Char(' '))) {
        simplifiedText.chop(1);
    }

    return simplifiedText;
}
