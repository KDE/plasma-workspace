/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <sessionsmodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    auto sessionsModel = new SessionsModel(&app);
    app.exec();
}
