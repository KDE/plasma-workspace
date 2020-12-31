/*
 *   Copyright 2008 Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "recentdocuments.h"

#include <QAction>
#include <QDir>
#include <QMimeData>

#include <KRun>
#include <KLocalizedString>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/Job>
#include <KShell>

#include <KActivities/Stats/ResultModel>
#include <KActivities/Stats/Terms>
#include <KActivities/Stats/Query>

using namespace KActivities::Stats;
using namespace KActivities::Stats::Terms;

K_EXPORT_PLASMA_RUNNER_WITH_JSON(RecentDocuments, "plasma-runner-recentdocuments.json")

RecentDocuments::RecentDocuments(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    setObjectName(QStringLiteral("Recent Documents"));

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Looks for documents recently used with names matching :q:.")));

    addAction(QStringLiteral("openParentDir"), QIcon::fromTheme(QStringLiteral("document-open-folder")), i18n("Open Containing Folder"));
}

RecentDocuments::~RecentDocuments()
{
}

void RecentDocuments::match(Plasma::RunnerContext &context)
{
    if (!context.isValid()) {
        return;
    }

    const QString term = context.query();
    if (term.length() < 3) {
        return;
    }

    auto query = UsedResources
            | Activity::current()
            | Order::RecentlyUsedFirst
            | Agent::any()
            // we search only on file name, as KActivity does not support better options
            | Url("/*/" + term + "*")
            | Limit(20);

    const auto result = new ResultModel(query);

    for (int i = 0; i < result->rowCount(); ++i) {
        const auto index = result->index(i, 0);

        const auto url = QUrl::fromUserInput(result->data(index, ResultModel::ResourceRole).toString(),
                                             QString(),
                                             // We can assume local file thanks to the request Url
                                             QUrl::AssumeLocalFile);
        const auto name = result->data(index, ResultModel::TitleRole).toString();

        Plasma::QueryMatch match(this);

        auto relevance = 0.5;
        match.setType(Plasma::QueryMatch::PossibleMatch);
        if (url.fileName() == term) {
            relevance = 1.0;
            match.setType(Plasma::QueryMatch::ExactMatch);
        } else if (url.fileName().startsWith(term)) {
            relevance = 0.9;
            match.setType(Plasma::QueryMatch::PossibleMatch);
        }
        match.setIconName(KIO::iconNameForUrl(url));
        match.setRelevance(relevance);
        match.setData(QVariant(url));
        match.setText(name);

        QString destUrlString = KShell::tildeCollapse(url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path());
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

    auto run = new KRun(url, nullptr);
    run->setRunExecutables(false);
}

QList<QAction *> RecentDocuments::actionsForMatch(const Plasma::QueryMatch &match)
{
    const QUrl url = match.data().toUrl();
    if (url.isLocalFile()) {
        return actions().values();
    }

    return {};
}

QMimeData * RecentDocuments::mimeDataForMatch(const Plasma::QueryMatch& match)
{
    QMimeData *result = new QMimeData();
    result->setUrls({match.data().toUrl()});
    return result;
}

#include "recentdocuments.moc"
