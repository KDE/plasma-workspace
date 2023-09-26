/*
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>

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
            | Type::files()
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
    QMimeDatabase db;
    QList<KRunner::QueryMatch> matches;
    for (int i = 0; i < m_resultsModel->rowCount(); ++i) {
        const QModelIndex index = m_resultsModel->index(i, 0);
        const QUrl url = QUrl::fromLocalFile(m_resultsModel->data(index, ResultModel::ResourceRole).toString());

        const QString fileName = url.fileName();
        const int indexOfTerm = fileName.indexOf(term, Qt::CaseInsensitive);
        if (indexOfTerm == -1) {
            continue; // A previous result or a result where the path, but not filename matches
        }

        KRunner::QueryMatch match(this);
        match.setRelevance(relevance);
        match.setType(KRunner::QueryMatch::CompletionMatch);
        // We know the term starts with the query, check size to see if it is an exact match
        if (term.size() >= 5 && indexOfTerm == 0 && (fileName.size() == term.size() || QFileInfo(fileName).baseName().size() == term.size())) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::ExactMatch);
        } else if (indexOfTerm == 0 /*startswith, but not equals => smaller relevance boost*/) {
            match.setRelevance(relevance + 0.1);
            match.setType(KRunner::QueryMatch::PossibleMatch);
        }
        const auto name = m_resultsModel->data(index, ResultModel::TitleRole).toString();
        const QMimeType mimeType = db.mimeTypeForName(m_resultsModel->data(index, ResultModel::MimeType).toString());
        match.setIconName(mimeType.iconName());
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
