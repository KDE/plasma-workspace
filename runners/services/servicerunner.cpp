/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <vhanda@kde.org>
    SPDX-FileCopyrightText: 2016-2020 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2022 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "servicerunner.h"

#include <algorithm>

#include <QMimeData>

#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

#include <KActivities/ResourceInstance>
#include <KActivities/Stats/Query>
#include <KActivities/Stats/ResultSet>
#include <KActivities/Stats/Terms>
#include <KApplicationTrader>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KServiceAction>
#include <KShell>
#include <KStringHandler>
#include <KSycoca>

#include <KIO/ApplicationLauncherJob>
#include <KIO/DesktopExecParser>

#include "debug.h"

namespace
{
int weightedLength(const QString &query)
{
    return KStringHandler::logicalLength(query);
}

inline bool contains(const QString &result, const QStringList &queryList)
{
    return std::all_of(queryList.cbegin(), queryList.cend(), [&result](const QString &query) {
        return result.contains(query, Qt::CaseInsensitive);
    });
}

inline bool contains(const QStringList &results, const QStringList &queryList)
{
    return std::all_of(queryList.cbegin(), queryList.cend(), [&results](const QString &query) {
        return std::any_of(results.cbegin(), results.cend(), [&query](const QString &result) {
            return result.contains(query, Qt::CaseInsensitive);
        });
    });
}

} // namespace

/**
 * @brief Finds all KServices for a given runner query
 */
class ServiceFinder
{
public:
    ServiceFinder(ServiceRunner *runner, const QList<KService::Ptr> &list, const QString &currentActivity)
        : m_runner(runner)
        , m_services(list)
        , m_currentActivity(currentActivity)
    {
    }

    void match(KRunner::RunnerContext &context)
    {
        query = context.query();
        // Splitting the query term to match using subsequences
        queryList = query.split(QLatin1Char(' '));
        weightedTermLength = weightedLength(query);

        matchNameKeywordAndGenericName();
        matchCategories();
        matchJumpListActions();

        context.addMatches(matches);
    }

private:
    void seen(const KService::Ptr &service)
    {
        m_seen.insert(service->storageId());
        m_seen.insert(service->exec());
    }

    void seen(const KServiceAction &action)
    {
        m_seen.insert(action.exec());
    }

    bool hasSeen(const KService::Ptr &service)
    {
        return m_seen.contains(service->storageId()) && m_seen.contains(service->exec());
    }

    bool hasSeen(const KServiceAction &action)
    {
        return m_seen.contains(action.exec());
    }

    bool disqualify(const KService::Ptr &service)
    {
        auto ret = hasSeen(service) || service->noDisplay();
        qCDebug(RUNNER_SERVICES) << service->name() << "disqualified?" << ret;
        seen(service);
        return ret;
    }

    qreal increaseMatchRelavance(const KService::Ptr &service, const QStringList &strList, const QString &category)
    {
        // Increment the relevance based on all the words (other than the first) of the query list
        qreal relevanceIncrement = 0;

        for (int i = 1; i < strList.size(); ++i) {
            const auto &str = strList.at(i);
            if (category == QLatin1String("Name")) {
                if (service->name().contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("GenericName")) {
                if (service->genericName().contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("Exec")) {
                if (service->exec().contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("Comment")) {
                if (service->comment().contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            }
        }

        return relevanceIncrement;
    }

    void setupMatch(const KService::Ptr &service, KRunner::QueryMatch &match)
    {
        const QString name = service->name();

        match.setText(name);

        QUrl url(service->storageId());
        url.setScheme(QStringLiteral("applications"));
        match.setData(url);

        match.setUrls({QUrl::fromLocalFile(service->entryPath())});

        QString exec = service->exec();

        QStringList resultingArgs = KIO::DesktopExecParser(KService(QString(), exec, QString()), {}).resultingArguments();

        // Remove any environment variables.
        if (KIO::DesktopExecParser::executableName(exec) == QLatin1String("env")) {
            resultingArgs.removeFirst(); // remove "env".

            while (!resultingArgs.isEmpty() && resultingArgs.first().contains(QLatin1Char('='))) {
                resultingArgs.removeFirst();
            }

            // Now parse it again to resolve the path.
            resultingArgs = KIO::DesktopExecParser(KService(QString(), KShell::joinArgs(resultingArgs), QString()), {}).resultingArguments();
        }

        // Remove special arguments that have no real impact on the application.
        static const auto specialArgs = {QStringLiteral("-qwindowtitle"), QStringLiteral("-qwindowicon"), QStringLiteral("--started-from-file")};

        for (const auto &specialArg : specialArgs) {
            int index = resultingArgs.indexOf(specialArg);
            if (index > -1) {
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index);
                }
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index); // remove value, too, if any.
                }
            }
        }

        match.setId(QStringLiteral("exec://") + resultingArgs.join(QLatin1Char(' ')));
        if (!service->genericName().isEmpty() && service->genericName() != name) {
            match.setSubtext(service->genericName());
        } else if (!service->comment().isEmpty()) {
            match.setSubtext(service->comment());
        }

        if (!service->icon().isEmpty()) {
            match.setIconName(service->icon());
        }
    }

    void matchNameKeywordAndGenericName()
    {
        const auto nameKeywordAndGenericNameFilter = [this](const KService::Ptr &service) {
            // Name
            if (contains(service->name(), queryList)) {
                return true;
            }
            // If the term length is < 3, no real point searching the Keywords and GenericName
            if (weightedTermLength < 3) {
                return false;
            }
            // Keywords
            if (contains(service->keywords(), queryList)) {
                return true;
            }
            // GenericName
            if (contains(service->genericName(), queryList) || contains(service->untranslatedGenericName(), queryList)) {
                return true;
            }
            // Comment
            if (contains(service->comment(), queryList)) {
                return true;
            }

            return false;
        };

        for (const KService::Ptr &service : m_services) {
            if (!nameKeywordAndGenericNameFilter(service) || disqualify(service)) {
                continue;
            }

            const QString id = service->storageId();
            const QString name = service->name();
            const QString exec = service->exec();

            KRunner::QueryMatch match(m_runner);
            match.setType(KRunner::QueryMatch::PossibleMatch);
            setupMatch(service, match);
            qreal relevance(0.6);

            // If the term was < 3 chars and NOT at the beginning of the App's name or Exec, then
            // chances are the user doesn't want that app.
            if (weightedTermLength < 3) {
                if (name.startsWith(query, Qt::CaseInsensitive) || exec.startsWith(query, Qt::CaseInsensitive)) {
                    relevance = 0.9;
                } else {
                    continue;
                }
            } else if (name.compare(query, Qt::CaseInsensitive) == 0) {
                relevance = 1;
                match.setType(KRunner::QueryMatch::ExactMatch);
            } else if (name.contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.8;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("Name"));

                if (name.startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.1;
                }
            } else if (service->genericName().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.65;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("GenericName"));

                if (service->genericName().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }
            } else if (service->comment().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.5;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("Comment"));

                if (service->comment().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }
            }

            if (service->categories().contains(QLatin1String("KDE"))) {
                qCDebug(RUNNER_SERVICES) << "found a kde thing" << id << match.subtext() << relevance;
                relevance += .09;
            }

            qCDebug(RUNNER_SERVICES) << name << "is this relevant:" << relevance;
            match.setRelevance(relevance);

            if (const auto foundIt = m_runner->m_favourites.constFind(service->desktopEntryName()); foundIt != m_runner->m_favourites.cend()) {
                if (foundIt->isGlobal || foundIt->linkedActivities.contains(m_currentActivity)) {
                    match.setRelevance(match.relevance() + 0.3);
                }
            }

            matches << match;
        }
    }

    void matchCategories()
    {
        // Do not match categories for short queries, BUG: 469769
        if (weightedTermLength < 5) {
            return;
        }
        const auto categoriesFilter = [this](const KService::Ptr &service) {
            return contains(service->categories(), queryList);
        };

        for (const KService::Ptr &service : m_services) {
            if (!categoriesFilter(service) || disqualify(service)) {
                continue;
            }
            qCDebug(RUNNER_SERVICES) << service->name() << "is an exact match!" << service->storageId() << service->exec();

            KRunner::QueryMatch match(m_runner);
            match.setType(KRunner::QueryMatch::PossibleMatch);
            setupMatch(service, match);

            qreal relevance = 0.4;
            const QStringList categories = service->categories();
            if (std::any_of(categories.begin(), categories.end(), [this](const QString &category) {
                    return category.compare(query, Qt::CaseInsensitive) == 0;
                })) {
                relevance = 0.6;
            } else if (service->categories().contains(QLatin1String("X-KDE-More")) || !service->showInCurrentDesktop()) {
                relevance = 0.5;
            }

            if (service->isApplication()) {
                relevance += .04;
            }

            match.setRelevance(relevance);
            matches << match;
        }
    }

    void matchJumpListActions()
    {
        if (weightedTermLength < 3) {
            return;
        }

        const auto hasActionsFilter = [](const KService::Ptr &service) {
            return !service->actions().isEmpty();
        };
        for (const KService::Ptr &service : m_services) {
            if (!hasActionsFilter(service) || service->noDisplay()) {
                continue;
            }

            // Skip SystemSettings as we find KCMs already
            if (service->storageId() == QLatin1String("systemsettings.desktop")) {
                continue;
            }

            const auto actions = service->actions();
            for (const KServiceAction &action : actions) {
                if (action.text().isEmpty() || action.exec().isEmpty() || hasSeen(action)) {
                    continue;
                }
                seen(action);

                const int matchIndex = action.text().indexOf(query, 0, Qt::CaseInsensitive);
                if (matchIndex < 0) {
                    continue;
                }

                KRunner::QueryMatch match(m_runner);
                match.setType(KRunner::QueryMatch::PossibleMatch);
                if (!action.icon().isEmpty()) {
                    match.setIconName(action.icon());
                } else {
                    match.setIconName(service->icon());
                }
                match.setText(i18nc("Jump list search result, %1 is action (eg. open new tab), %2 is application (eg. browser)",
                                    "%1 - %2",
                                    action.text(),
                                    service->name()));

                QUrl url(service->storageId());
                url.setScheme(QStringLiteral("applications"));

                QUrlQuery urlQuery;
                urlQuery.addQueryItem(QStringLiteral("action"), action.name());
                url.setQuery(urlQuery);

                match.setData(url);

                qreal relevance = 0.5;
                if (action.text().compare(query, Qt::CaseInsensitive) == 0) {
                    relevance = 0.65;
                    match.setType(KRunner::QueryMatch::HelperMatch); // Give it a higer match type to ensure it is shown, BUG: 455436
                } else if (matchIndex == 0) {
                    relevance += 0.05;
                }

                match.setRelevance(relevance);

                matches << match;
            }
        }
    }

    ServiceRunner *m_runner;
    QSet<QString> m_seen;
    const QList<KService::Ptr> m_services;
    const QString m_currentActivity;

    QList<KRunner::QueryMatch> matches;
    QString query;
    QStringList queryList;
    int weightedTermLength = -1;
};

ServiceRunner::ServiceRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_kactivitiesQuery(Terms::LinkedResources | Terms::Agent{QStringLiteral("org.kde.plasma.favorites.applications")} | Terms::Type::any()
                         | Terms::Activity::any() | Terms::Limit::all())
    , m_kactivitiesWatcher(m_kactivitiesQuery)
{
    addSyntax(QStringLiteral(":q:"), i18n("Finds applications whose name or description match :q:"));
    connect(&m_kactivitiesWatcher, &ResultWatcher::resultLinked, [this](const QString &resource) {
        processActivitiesResults(ResultSet(m_kactivitiesQuery | Terms::Url::contains(resource)));
    });

    connect(&m_kactivitiesWatcher, &ResultWatcher::resultUnlinked, [this](const QString &resource) {
        m_favourites.remove(resource);
        // In case it was only unlinked from one activity
        processActivitiesResults(ResultSet(m_kactivitiesQuery | Terms::Url::contains(resource)));
    });
    processActivitiesResults(ResultSet(m_kactivitiesQuery));

    // Load services once per match session. Reloading them for every character is not worth it
    connect(this, &KRunner::AbstractRunner::prepare, this, [this]() {
        m_services = KApplicationTrader::query([](const KService::Ptr &) {
            return true;
        });
    });
    connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
        m_services.clear();
    });
}

void ServiceRunner::processActivitiesResults(const ResultSet &results)
{
    const static QLatin1String globalActivity(":global");
    const static QLatin1String applicationScheme("applications");
    for (const ResultSet::Result &result : results) {
        if (result.url().scheme() == applicationScheme) {
            m_favourites.insert(result.url().path().remove(QLatin1String(".desktop")),
                                ActivityFavourite{
                                    result.linkedActivities(),
                                    result.linkedActivities().contains(globalActivity),
                                });
        }
    }
}

void ServiceRunner::match(KRunner::RunnerContext &context)
{
    ServiceFinder finder(this, m_services, m_activitiesConsuer.currentActivity());
    finder.match(context);
}

void ServiceRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const QUrl dataUrl = match.data().toUrl();

    KService::Ptr service = KService::serviceByStorageId(dataUrl.path());
    if (!service) {
        return;
    }

    KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("applications:") + service->storageId()), QStringLiteral("org.kde.krunner"));

    KIO::ApplicationLauncherJob *job = nullptr;

    const QString actionName = QUrlQuery(dataUrl).queryItemValue(QStringLiteral("action"));
    if (actionName.isEmpty()) {
        job = new KIO::ApplicationLauncherJob(service);
    } else {
        const auto actions = service->actions();
        auto it = std::find_if(actions.begin(), actions.end(), [&actionName](const KServiceAction &action) {
            return action.name() == actionName;
        });
        Q_ASSERT(it != actions.end());

        job = new KIO::ApplicationLauncherJob(*it);
    }

    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->start();
}

K_PLUGIN_CLASS_WITH_JSON(ServiceRunner, "plasma-runner-services.json")

#include "servicerunner.moc"
