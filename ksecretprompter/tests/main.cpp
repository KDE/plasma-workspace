/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "promptertest.h"

#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain("ksecretpromptertest");

    PrompterTest test;

    return application.exec();
}
