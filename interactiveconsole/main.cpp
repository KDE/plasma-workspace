/*
    SPDX-FileCopyrightText: 2021 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "interactiveconsole.h"
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    // set to delete on close
    auto console = new InteractiveConsole;
    console->show();
    app.exec();
}
