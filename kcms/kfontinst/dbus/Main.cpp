/*
    SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontInst.h"
#include "Misc.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    // KJob uses a QEventLoopLocker which causes kfontinst to quit
    // after the job is done, prevent this by disabling quit lock.
    QCoreApplication::setQuitLockEnabled(false);

    QCoreApplication *app = new QCoreApplication(argc, argv);
    KFI::FontInst fi;

    int rv = app->exec();
    delete app;
    return rv;
}
