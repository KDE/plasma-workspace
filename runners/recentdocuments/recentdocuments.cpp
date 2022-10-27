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
#include <KActivities/Stats/ResultModel>
#include <KActivities/Stats/Terms>
#include <qnamespace.h>

using namespace KActivities::Stats;
using namespace KActivities::Stats::Terms;

K_PLUGIN_CLASS_WITH_JSON(RecentDocuments, "plasma-runner-recentdocuments.json")

RecentDocuments::RecentDocuments(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("Recent Documents"));

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Looks for documents recently used with names matching :q:.")));

    m_actions = {new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Open Containing Folder"), this)};
    setMinLetterCount(3);
}

RecentDocuments::~RecentDocuments()
{
}

void RecentDocuments::match(Plasma::RunnerContext &context)
{
    if (!context.isValid()) {
        return;
    }

    // clang-format off
    const QString term = context.query();
    auto query = UsedResources
            | Activity::current()
            | Order::RecentlyUsedFirst
            | Agent::any()
            // we search only on file name, as KActivity does not support better options
            | Url("*" + term + "*")
            | Limit(20);
    // clang-format on

    const auto result = new ResultModel(query);

    float relevance = 0.75;
    Plasma::QueryMatch::Type type = Plasma::QueryMatch::CompletionMatch;
    for (int i = 0; i < result->rowCount(); ++i) {
        const auto index = result->index(i, 0);

        QString path = result->data(index, ResultModel::ResourceRole).toString();
        // If the matching term is only in the path and not in the file name, use as the result the directory where the match occurs, i.e. if query "a" matches
        // file "a/b.txt", return directory "a"
        int endMatchingDir = path.indexOf(QStringLiteral("/"), path.lastIndexOf(term, -1, Qt::CaseInsensitive) + term.length());
        if (endMatchingDir > -1) {
            path.truncate(endMatchingDir);
        }

        const QUrl url = QUrl::fromUserInput(path,
                                             QString(),
                                             // We can assume local file thanks to the request Url
                                             QUrl::AssumeLocalFile);
        const QString name = url.fileName();

        Plasma::QueryMatch match(this);

        match.setRelevance(relevance);
        match.setType(type);
        if (term.size() >= 5
            && (url.fileName().compare(term, Qt::CaseInsensitive) == 0 || QFileInfo(url.fileName()).baseName().compare(term, Qt::CaseInsensitive) == 0)) {
            match.setRelevance(relevance + 0.1);
            match.setType(Plasma::QueryMatch::ExactMatch);
        } else if (url.fileName().startsWith(term, Qt::CaseInsensitive)) {
            match.setRelevance(relevance + 0.1);
            match.setType(Plasma::QueryMatch::PossibleMatch);
        } else if (!url.fileName().contains(term)) {
            continue;
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

        context.addMatch(match);
    }
}

void RecentDocuments::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

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
