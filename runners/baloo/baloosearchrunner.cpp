/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "baloosearchrunner.h"

#include <KLocalizedString>
#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDir>
#include <QIcon>
#include <QMimeData>
#include <QMimeDatabase>
#include <QTimer>

#include <Baloo/IndexerConfig>
#include <Baloo/Query>

#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KNotificationJobUiDelegate>
#include <KShell>

#include "krunner1adaptor.h"

static const QString s_openParentDirId = QStringLiteral("openParentDir");

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv); // KRun needs widgets for error message boxes
    SearchRunner r;
    return app.exec();
}

SearchRunner::SearchRunner(QObject *parent)
    : QObject(parent)
{
    new Krunner1Adaptor(this);
    qDBusRegisterMetaType<RemoteMatch>();
    qDBusRegisterMetaType<RemoteMatches>();
    qDBusRegisterMetaType<RemoteAction>();
    qDBusRegisterMetaType<RemoteActions>();
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/runner"), this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.runners.baloo"));
}

SearchRunner::~SearchRunner()
{
}

RemoteActions SearchRunner::Actions()
{
    Baloo::IndexerConfig config;
    if (!config.fileIndexingEnabled()) {
        sendErrorReply(QDBusError::ErrorType::NotSupported);
    }
    return RemoteActions({RemoteAction{s_openParentDirId, i18n("Open Containing Folder"), QStringLiteral("document-open-folder")}});
}

RemoteMatches SearchRunner::Match(const QString &searchTerm)
{
    Baloo::IndexerConfig config;
    if (!config.fileIndexingEnabled()) {
        sendErrorReply(QDBusError::ErrorType::NotSupported);
        return {};
    }

    // Do not try to show results for queries starting with =
    // this should trigger the calculator, but the AdvancedQueryParser::parse method
    // in baloo interpreted it as an operator, BUG 345134
    if (searchTerm.startsWith(QLatin1Char('='))) {
        return RemoteMatches();
    }

    // Filter out duplicates
    QSet<QUrl> foundUrls;

    RemoteMatches matches;
    matches << matchInternal(searchTerm, QStringLiteral("Audio"), i18n("Audio"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Image"), i18n("Image"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Video"), i18n("Video"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Spreadsheet"), i18n("Spreadsheet"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Presentation"), i18n("Presentation"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Folder"), i18n("Folder"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Document"), i18n("Document"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Archive"), i18n("Archive"), foundUrls);
    matches << matchInternal(searchTerm, QStringLiteral("Text"), i18n("Text"), foundUrls);

    return matches;
}

RemoteMatches SearchRunner::matchInternal(const QString &searchTerm, const QString &type, const QString &category, QSet<QUrl> &foundUrls)
{
    Baloo::Query query;
    query.setSearchString(searchTerm);
    query.setType(type);
    query.setLimit(10);

    Baloo::ResultIterator it = query.exec();

    RemoteMatches matches;

    QMimeDatabase mimeDb;

    // KRunner is absolutely daft and allows plugins to set the global
    // relevance levels. so Baloo should not set the relevance of results too
    // high because then Applications will often appear after if the application
    // runner has not a higher relevance. So stupid.
    // Each runner plugin should not have to know about the others.
    // Anyway, that's why we're starting with .75
    float relevance = .75;
    while (it.next()) {
        RemoteMatch match;
        QString localUrl = it.filePath();
        const QUrl url = QUrl::fromLocalFile(localUrl);

        if (foundUrls.contains(url)) {
            continue;
        }

        foundUrls.insert(url);

        match.id = url.toString();
        match.text = url.fileName();
        match.iconName = mimeDb.mimeTypeForFile(localUrl).iconName();
        match.relevance = relevance;
        match.type =
            url.fileName().compare(searchTerm, Qt::CaseInsensitive) == 0 || QFileInfo(url.fileName()).baseName().compare(searchTerm, Qt::CaseInsensitive) == 0
            ? Plasma::QueryMatch::ExactMatch
            : url.fileName().contains(searchTerm, Qt::CaseInsensitive) ? Plasma::QueryMatch::PossibleMatch
                                                                       : Plasma::QueryMatch::CompletionMatch;
        QVariantMap properties;

        QString folderPath = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toLocalFile();
        folderPath = KShell::tildeCollapse(folderPath);

        properties[QStringLiteral("urls")] = QStringList({QString::fromLocal8Bit(url.toEncoded())});
        properties[QStringLiteral("subtext")] = folderPath;
        properties[QStringLiteral("category")] = category;

        match.properties = properties;
        relevance -= 0.05;

        matches << match;
    }

    return matches;
}

void SearchRunner::Run(const QString &id, const QString &actionId)
{
    const QUrl url(id);
    if (actionId == s_openParentDirId) {
        KIO::highlightInFileManager({url});
        return;
    }

    auto *job = new KIO::OpenUrlJob(url);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, QApplication::activeWindow()));
    job->setShowOpenOrExecuteDialog(true);
    job->start();
}
