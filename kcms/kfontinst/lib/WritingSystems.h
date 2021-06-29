#ifndef __WRITING_SYSTEMS_H__
#define __WRITING_SYSTEMS_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfontinst_export.h"
#include <QMap>
#include <QStringList>
#include <fontconfig/fontconfig.h>

namespace KFI
{
class KFONTINST_EXPORT WritingSystems
{
public:
    static WritingSystems *instance();

    WritingSystems();

    qulonglong get(FcPattern *pat) const;
    qulonglong get(const QStringList &langs) const;
    QStringList getLangs(qulonglong ws) const;

private:
    QMap<QString, qulonglong> itsMap;
};

}

#endif
