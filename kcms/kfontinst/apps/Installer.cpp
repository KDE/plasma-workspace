/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Installer.h"
#include "FontsPackage.h"
#include "JobRunner.h"
#include "Misc.h"
#include "config-workspace.h"
#include <KAboutData>
#include <KIO/StatJob>
#include <KMessageBox>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QTemporaryDir>

// This include must be at the end
#include "CreateParent.h"

namespace KFI
{
int CInstaller::install(const QSet<QUrl> &urls)
{
    QSet<QUrl>::ConstIterator it(urls.begin()), end(urls.end());
    bool sysInstall(false);
    CJobRunner *jobRunner = new CJobRunner(m_parent);

    CJobRunner::startDbusService();

    if (!Misc::root()) {
        switch (KMessageBox::questionTwoActionsCancel(m_parent,
                                                      i18n("Do you wish to install the font(s) for personal use "
                                                           "(only available to you), or "
                                                           "system-wide (available to all users)?"),
                                                      i18n("Where to Install"),
                                                      KGuiItem(KFI_KIO_FONTS_USER.toString()),
                                                      KGuiItem(KFI_KIO_FONTS_SYS.toString()))) {
        case KMessageBox::SecondaryAction:
            sysInstall = true;
            break;
        case KMessageBox::Cancel:
            return -1;
        default:
            break;
        }
    }

    QSet<QUrl> instUrls;

    for (; it != end; ++it) {
        auto job = KIO::mostLocalUrl(*it);
        job->exec();
        QUrl local = job->mostLocalUrl();
        bool package(false);

        if (local.isLocalFile()) {
            QString localFile(local.toLocalFile());

            if (Misc::isPackage(localFile)) {
                instUrls += FontsPackage::extract(localFile, &m_tempDir);
                package = true;
            }
        }
        if (!package) {
            QList<QUrl> associatedUrls;

            CJobRunner::getAssociatedUrls(*it, associatedUrls, false, m_parent);
            instUrls.insert(*it);

            QList<QUrl>::Iterator aIt(associatedUrls.begin()), aEnd(associatedUrls.end());

            for (; aIt != aEnd; ++aIt) {
                instUrls.insert(*aIt);
            }
        }
    }

    if (!instUrls.isEmpty()) {
        CJobRunner::ItemList list;
        QSet<QUrl>::ConstIterator it(instUrls.begin()), end(instUrls.end());

        for (; it != end; ++it) {
            list.append(*it);
        }

        return jobRunner->exec(CJobRunner::CMD_INSTALL, list, Misc::root() || sysInstall);
    } else {
        return -1;
    }
}

CInstaller::~CInstaller()
{
    delete m_tempDir;
}

}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KLocalizedString::setApplicationDomain(KFI_CATALOGUE);
    KAboutData aboutData("kfontinst",
                         i18n("Font Installer"),
                         WORKSPACE_VERSION_STRING,
                         i18n("Simple font installer"),
                         KAboutLicense::GPL,
                         i18n("(C) Craig Drummond, 2007"));
    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setWindowIcon(QIcon::fromTheme("preferences-desktop-font-installer"));

    QCommandLineParser parser;
    const QCommandLineOption embedOption(QLatin1String("embed"), i18n("Makes the dialog transient for an X app specified by winid"), QLatin1String("winid"));
    parser.addOption(embedOption);
    parser.addPositionalArgument(QLatin1String("[URL]"), i18n("URL to install"));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QSet<QUrl> urls;

    foreach (const QString &arg, parser.positionalArguments())
        urls.insert(QUrl::fromUserInput(arg, QDir::currentPath()));

    if (!urls.isEmpty()) {
        QString opt(parser.value(embedOption));
        KFI::CInstaller inst(createParent(opt.size() ? opt.toInt(nullptr, 16) : 0));

        return inst.install(urls);
    }

    return -1;
}
