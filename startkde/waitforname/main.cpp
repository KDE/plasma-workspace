/*
    SPDX-FileCopyrightText: 2017 Valerio Pilo <vpilo@coldshock.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "waiter.h"

int main(int argc, char **argv)
{
    Waiter app(argc, argv);

    if (!app.waitForService()) {
        return 0;
    }

    return app.exec();
}
