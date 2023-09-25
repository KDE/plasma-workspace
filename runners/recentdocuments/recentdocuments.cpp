/*
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "recentdocuments.h"

#include <QApplication>
#include <QDir>
#include <QMimeData>

#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KShell>

#include <KActivities/Stats/Query>
#include <KActivities/Stats/Terms>
#include <kactivitiesstats/resultset.h>

using namespace KActivities::Stats::Terms;

K_PLUGIN_CLASS_WITH_JSON(RecentDocuments, "plasma-runner-recentdocuments.json")

RecentDocuments::RecentDocuments(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_actions({KRunner::Action(QStringLiteral("open-folder"), QStringLiteral("document-open-folder"), i18n("Open Containing Folder"))})
{
    addSyntax(QStringLiteral(":q:"), i18n("Looks for documents recently used with names matching :q:."));

    setMinLetterCount(3);
    connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
        m_resultsModel.reset(nullptr);
    });
}

void RecentDocuments::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    if (!m_resultsModel || !term.startsWith(m_lastLoadedQuery)) {
        auto query = UsedResources //
            | Activity::current() //
            | Order::RecentlyUsedFirst //
            | Agent::any()
            // we search only on file name, as KActivity does not support better options
            | Url("/*/*" + term + "*");

        // Reuse the model in case our query starts with the previous one. We filter out irrelevant results later on anyway
        m_resultsModel.reset(new ResultModel(query));
        m_lastLoadedQuery = term;
    }

    if (!context.isValid()) {
        return; // The initial fetching could take a moment, check the context validity afterward
    }
    float relevance = 0.75;
    KRunner::QueryMatch::Type type = KRunner::QueryMatch::CompletionMatch;
    QList<KRunner::QueryMatch> matches;
    for (int i = 0; i < m_resultsModel->rowCount(); ++i) {
        const auto index = m_resultsModel->index(i, 0);

        const auto url = QUrl::fromUserInput(m_resultsModel->data(index, ResultModel::ResourceRole).toString(),
                                             QString(),
                                             // We can assume local file thanks to the request Url
                                             QUrl::AssumeLocalFile);
        const auto name = m_resultsModel->data(index, ResultModel::TitleRole).toString();

        KRunner::QueryMatch match(this);

        match.setRelevance(relevance);
        match.setType(type);
        const QString fileName = url.fileName();
        // In case our filename starts with the query, we only need to check the size to check if it is an exact match/exact match of the basename
        const int indexOfTerm = fileName.indexOf(term, Qt::CaseInsensitive);
        const bool startsWithTerm = indexOfTerm == 0;
        if (indexOfTerm == -1) {
            continue;
        } else if (term.size() >= 5 && startsWithTerm && (fileName.size() == term.size() || QFileInfo(fileName).baseName().size() == term.size())) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::ExactMatch);
        } else if (startsWithTerm) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::PossibleMatch);
        }
        match.setIconName(KIO::iconNameForUrl(url));
        match.setData(QVariant(url));
        match.setUrls({url});
        match.setId(url.toString());
        if (url.isLocalFile()) {
            match.setActions(m_actions);
        }
        match.setText(name);
        QString destUrlString = KShell::tildeCollapse(url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path());
        match.setSubtext(destUrlString);

        relevance -= 0.05;
        matches << match;
    }
    context.addMatches(matches);
}

void RecentDocuments::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const QUrl url = match.data().toUrl();

    if (match.selectedAction()) {
        KIO::highlightInFileManager({url});
        return;
    }

    auto *job = new KIO::OpenUrlJob(url);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, QApplication::activeWindow()));
    job->setShowOpenOrExecuteDialog(true);
    job->start();
}

#include "recentdocuments.moc"
