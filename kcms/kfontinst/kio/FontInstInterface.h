#ifndef __FONTINST_INTERFACE_H__
#define __FONTINST_INTERFACE_H__

/*
 * KFontInst - KDE Font Installer
 *
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "Family.h"
#include <QEventLoop>
#include <QObject>

class OrgKdeFontinstInterface;

namespace KFI
{
class FontInstInterface : public QObject
{
    Q_OBJECT

public:
    FontInstInterface();
    ~FontInstInterface() override;

    int install(const QString &file, bool toSystem);
    int uninstall(const QString &name, bool fromSystem);
    int reconfigure();
    Families list(bool system);
    Family statFont(const QString &file, bool system);
    QString folderName(bool sys);

private:
    int waitForResponse();

private Q_SLOTS:

    void dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to);
    void status(int pid, int value);
    void fontList(int pid, const QList<KFI::Families> &families);
    void fontStat(int pid, const KFI::Family &font);

private:
    OrgKdeFontinstInterface *itsInterface;
    bool itsActive;
    int itsStatus;
    Families itsFamilies;
    QEventLoop itsEventLoop;
};

}

#endif
