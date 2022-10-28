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

    const QStringList terms = context.query().split(QLatin1Char(' '));

    QList<QUrl> urls;
    // fetch results for each query word
    for (const QString &term : terms) {
        // clang-format off
        auto query = UsedResources
                | Activity::current()
                | Order::RecentlyUsedFirst
                | Agent::any()
                // we search only on file name, as KActivity does not support better options
                | Url("*" + term + "*")
                | Limit(20);
        // clang-format on

        const auto result = new ResultModel(query);
        for (int i = 0; i < result->rowCount(); ++i) {
            const auto index = result->index(i, 0);
            const auto url = QUrl::fromUserInput(result->data(index, ResultModel::ResourceRole).toString(),
                                                 QString(),
                                                 // We can assume local file thanks to the request Url
                                                 QUrl::AssumeLocalFile);

            urls << url;
        }
    }

    // filter the files so that the file name matchse all of the query words
    for (const QUrl &url : urls) {
        const QString name = url.fileName();

        Plasma::QueryMatch match(this);

        if (terms.size() >= 5 && name == context.query()) {
            match.setRelevance(1.0);
            match.setType(Plasma::QueryMatch::ExactMatch);
        } else if (name.startsWith(context.query())) {
            match.setRelevance(0.9);
            match.setType(Plasma::QueryMatch::PossibleMatch);
        } else if (std::all_of(terms.cbegin(), terms.cend(), [&name](const QString &term) {
                       return name.contains(term, Qt::CaseInsensitive);
                   })) {
            match.setRelevance(0.5);
            match.setType(Plasma::QueryMatch::CompletionMatch);
        } else {
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
        const QString destUrlString = KShell::tildeCollapse(url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path());
        match.setSubtext(destUrlString);

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
