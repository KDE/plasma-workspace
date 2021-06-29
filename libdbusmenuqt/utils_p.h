/* This file is part of the dbusmenu-qt library
    SPDX-FileCopyrightText: 2010 Canonical
    Author: Aurelien Gateau <aurelien.gateau@canonical.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef UTILS_P_H
#define UTILS_P_H

class QString;

/**
 * Swap mnemonic char: Qt uses '&', while dbusmenu uses '_'
 */
QString swapMnemonicChar(const QString &in, const char src, const char dst);

#endif /* UTILS_P_H */
