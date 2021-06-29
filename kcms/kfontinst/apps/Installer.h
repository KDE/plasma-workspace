#ifndef __INSTALLER_H__
#define __INSTALLER_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSet>
#include <QUrl>

class QWidget;
class QTemporaryDir;

namespace KFI
{
class CInstaller
{
public:
    CInstaller(QWidget *p)
        : itsParent(p)
        , itsTempDir(nullptr)
    {
    }
    ~CInstaller();

    int install(const QSet<QUrl> &urls);

private:
    QWidget *itsParent;
    QTemporaryDir *itsTempDir;
};

}

#endif
