/*
    SPDX-FileCopyrightText: 2024 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "ksecretprompter.h"

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    application.setQuitLockEnabled(false);
    application.setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain("ksecretprompter");

    KAboutData aboutData(u"secretprompter"_s,
                         QString(), // No application name to not append ksecretprompter to dialog window titles
                         QStringLiteral(PROJECT_VERSION),
                         i18nc("application description", "A service to show password prompts"),
                         KAboutLicense::GPL,
                         i18nc("copyright statement", "Copyright 2025, Marco Martin <notmart@gmail.com>"));

    aboutData.addAuthor(i18nc("author name", "Marco Martin"), i18nc("task description for an author line in about data", "Author"), QString());

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
    parser.addOption(replaceOption);
    parser.process(application);
    aboutData.processCommandLine(&parser);

    const bool replace = parser.isSet(replaceOption);

    KCrash::initialize();

    KDBusService dbusUniqueInstance(KDBusService::Unique | KDBusService::StartupOption(replace ? KDBusService::Replace : 0));

    KSecretPrompter prompter;

    return application.exec();
}
