/*
    SPDX-FileCopyrightText: 2017 Valerio Pilo <vpilo@coldshock.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "waiter.h"
#include <signal.h>

void sigtermHandler(int signalNumber)
{
    Q_UNUSED(signalNumber)
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->exit(-1);
    }
}
int main(int argc, char **argv)
{
    Waiter app(argc, argv);
    signal(SIGTERM, sigtermHandler);

    if (!app.waitForService()) {
        return 0;
    }

    return app.exec();
}
