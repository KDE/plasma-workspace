/*
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "recentdocuments.h"

#include <QApplication>
#include <QDir>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>

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

using namespace KActivities::Stats::Terms;

K_PLUGIN_CLASS_WITH_JSON(RecentDocuments, "plasma-runner-recentdocuments.json")

RecentDocuments::RecentDocuments(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_actions({KRunner::Action(QStringLiteral("open-folder"), QStringLiteral("document-open-folder"), i18n("Open Containing Folder"))})
{
    addSyntax(QStringLiteral(":q:"), i18n("Looks for documents recently used with names matching :q:."));

    setMinLetterCount(m_minLetterCount);
    connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
        m_resultsModel.reset(nullptr);
    });
}

void RecentDocuments::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();

    if (!m_resultsModel || m_resultsModel->rowCount() == m_maxResults || m_lastLoadedQuery.size() < m_minLetterCount || !term.startsWith(m_lastLoadedQuery)) {
        const QLatin1String asterix("*");
        const QString termPattern = (term.size() < m_minLetterCount ? QLatin1String() : asterix) + term + asterix;
        auto query = UsedResources | Activity::current() | Order::RecentlyUsedFirst | Agent::any() //
            | Type::files() // Only show files and not folders
            | Limit(m_maxResults) // In case we are in single runner mode, we could get tons of results for one or two letter queries
            | Url("/*/*") // we search only for local files
            | Title({termPattern}); // check the title, because that is the filename

        // Reuse the model in case our query starts with the previous one. We filter out irrelevant results later on anyway
        m_resultsModel.reset(new ResultModel(query));
        m_lastLoadedQuery = term;
    }

    if (!context.isValid()) {
        return; // The initial fetching could take a moment, check the context validity afterward
    }
    float relevance = 0.75;
    QMimeDatabase db;
    QList<KRunner::QueryMatch> matches;
    for (int i = 0; i < m_resultsModel->rowCount(); ++i) {
        const QModelIndex index = m_resultsModel->index(i, 0);

        const auto fileName = m_resultsModel->data(index, ResultModel::TitleRole).toString();
        const int indexOfTerm = fileName.indexOf(term, Qt::CaseInsensitive);
        if (indexOfTerm == -1) {
            continue; // A previous result or a result where the path, but not filename matches
        }

        KRunner::QueryMatch match(this);
        // We know the term starts with the query, check size to see if it is an exact match
        if (term.size() >= 5 && indexOfTerm == 0 && (fileName.size() == term.size() || QFileInfo(fileName).baseName().size() == term.size())) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::ExactMatch);
        } else if (indexOfTerm == 0 /*startswith, but not equals => smaller relevance boost*/) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::PossibleMatch);
        } else {
            match.setRelevance(relevance);
            match.setType(KRunner::QueryMatch::CompletionMatch);
        }
        const QMimeType mimeType = db.mimeTypeForName(m_resultsModel->data(index, ResultModel::MimeType).toString());
        match.setIconName(mimeType.iconName());
        const QUrl url = QUrl::fromLocalFile(m_resultsModel->data(index, ResultModel::ResourceRole).toString());
        match.setData(QVariant(url));
        match.setUrls({url});
        match.setId(url.toString());
        if (url.isLocalFile()) {
            match.setActions(m_actions);
        }
        match.setText(fileName);
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
