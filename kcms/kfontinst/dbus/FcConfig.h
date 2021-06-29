#ifndef __FC_CONFIG_H__
#define __FC_CONFIG_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

class QString;

namespace KFI
{
namespace FcConfig
{
void addDir(const QString &dir, bool system);
}

}

#endif
