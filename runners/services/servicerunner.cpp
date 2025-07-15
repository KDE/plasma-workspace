/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <vhanda@kde.org>
    SPDX-FileCopyrightText: 2016-2025 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2022-2023 Alexander Lohnau <alexander.lohnau@gmx.de>

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

#include <KApplicationTrader>
#include <KFuzzyMatcher>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KServiceAction>
#include <KShell>
#include <KStringHandler>
#include <KSycoca>
#include <PlasmaActivities/ResourceInstance>

#include <KIO/ApplicationLauncherJob>
#include <KIO/DesktopExecParser>

#include "debug.h"

using namespace Qt::StringLiterals;

namespace
{

#include "fzf.cpp"

int weightedLength(const QString &query)
{
    return KStringHandler::logicalLength(query);
}

inline bool contains(const QStringList &results, const QList<QStringView> &queryList)
{
    return std::ranges::all_of(queryList, [&results](QStringView query) {
        return std::ranges::any_of(results, [&query](QStringView result) {
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
    ServiceFinder(ServiceRunner *runner, const QList<KService::Ptr> &list)
        : m_runner(runner)
        , m_services(list)
    {
    }

    void match(KRunner::RunnerContext &context)
    {
        query = context.query();
        // Splitting the query term to match using subsequences
        queryList = QStringView(query).split(QLatin1Char(' '));
        weightedTermLength = weightedLength(query);

        matchNameKeywordAndGenericName();
        matchCategories();
        matchJumpListActions();

        context.addMatches(matches);
    }

private:
    void seen(const KService::Ptr &service)
    {
        m_seen.insert(service->exec());
    }

    void seen(const KServiceAction &action)
    {
        m_seen.insert(action.exec());
    }

    bool hasSeen(const KService::Ptr &service)
    {
        return m_seen.contains(service->exec());
    }

    bool hasSeen(const KServiceAction &action)
    {
        return m_seen.contains(action.exec());
    }

    bool disqualify(const KService::Ptr &service)
    {
        auto ret = hasSeen(service);
        qCDebug(RUNNER_SERVICES) << service->name() << "disqualified?" << ret;
        seen(service);
        return ret;
    }

    void setupMatch(const KService::Ptr &service, KRunner::QueryMatch &match)
    {
        const QString name = service->name();
        const QString exec = service->exec();

        // FIXME huge hacky to support another huge hacky in fzf matching
        if (match.text().isEmpty()) {
            match.setText(name);
        }

        QUrl url(service->storageId());
        url.setScheme(QStringLiteral("applications"));
        match.setData(url);
        match.setUrls({QUrl::fromLocalFile(service->entryPath())});

        QString urlPath = resolvedArgs(exec);
        if (urlPath.isEmpty()) {
            // Otherwise we might filter out broken services. Rather than hiding them, it is better to show an error message on launch (as done by KIO's jobs)
            urlPath = exec;
        }
        match.setId(QString(u"exec://" + urlPath));
        if (!service->genericName().isEmpty() && service->genericName() != name) {
            match.setSubtext(service->genericName());
        } else if (!service->comment().isEmpty()) {
            match.setSubtext(service->comment());
        }

        if (!service->icon().isEmpty()) {
            match.setIconName(service->icon());
        }
    }

    QString resolvedArgs(const QString &exec)
    {
        const KService syntheticService(QString(), exec, QString());
        KIO::DesktopExecParser parser(syntheticService, {});
        QStringList resultingArgs = parser.resultingArguments();
        if (const auto error = parser.errorMessage(); resultingArgs.isEmpty() && !error.isEmpty()) {
            qCWarning(RUNNER_SERVICES) << "Failed to resolve executable from service. Error:" << error;
            return QString();
        }

        // Remove any environment variables.
        if (KIO::DesktopExecParser::executableName(exec) == QLatin1String("env")) {
            resultingArgs.removeFirst(); // remove "env".

            while (!resultingArgs.isEmpty() && resultingArgs.first().contains(QLatin1Char('='))) {
                resultingArgs.removeFirst();
            }

            // Now parse it again to resolve the path.
            resultingArgs = KIO::DesktopExecParser(KService(QString(), KShell::joinArgs(resultingArgs), QString()), {}).resultingArguments();
            return resultingArgs.join(QLatin1Char(' '));
        }

        // Remove special arguments that have no real impact on the application.
        static const auto specialArgs = {QStringLiteral("-qwindowtitle"), QStringLiteral("-qwindowicon"), QStringLiteral("--started-from-file")};

        for (const auto &specialArg : specialArgs) {
            auto index = resultingArgs.indexOf(specialArg);
            if (index > -1) {
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index);
                }
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index); // remove value, too, if any.
                }
            }
        }
        return resultingArgs.join(QLatin1Char(' '));
    }

    struct Score {
        int value;
        KRunner::QueryMatch::CategoryRelevance categoryRelevance;
    };
    [[nodiscard]] std::optional<Score> fuzzyScore(KService::Ptr service, KRunner::QueryMatch &match)
    {
        if (queryList.isEmpty()) {
            return std::nullopt; // No query, no score.
        }

        const auto name = service->name();

        // Absolute match. Can't get any better than this.
        if (name.compare(query, Qt::CaseInsensitive) == 0) {
            return Score{.value = std::numeric_limits<decltype(Score::value)>::max(), .categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Highest};
        }

        auto makeScore = [this, service, &match](const auto &string) {
            qreal score = 0;
            for (const auto &queryItem : queryList) {
                if (qEnvironmentVariableIntValue("FZF") == 1) {
                    // FIXME converts queryItem to string; it shouldn't
                    // FIXME make this an option enum we can init by name so the bools become obvious
                    auto x = FuzzyMatchV2(false, true, true, string.toLower(), queryItem.toString().toLower(), true);
                    auto result = x.first;
                    auto pos = x.second;
                    if (result.Score > 0) {
                        score += result.Score;
                        if (string.startsWith(queryItem, Qt::CaseInsensitive)) {
                            score *= 2;
                        }

                        // FIXME huge hacky to suggest where the match is
                        if (pos) {
                            QString text = string;
                            for (auto it = pos->begin(); it != pos->end(); ++it) {
                                const auto index = *it;
                                text.insert(index + 1, QChar(0x0331));
                            }
                            if (match.text().isEmpty()) {
                                match.setText(text);
                            } else {
                                match.setText(match.text() + " + "_L1 + text);
                            }
                        }
                    }
                } else {
                    if (auto result = KFuzzyMatcher::match(queryItem, string); [&] {
                            qDebug() << "KFuzzyMatcher:" << queryItem << "against" << string << "resulted in" << result.matched << "with score" << result.score;
                            return result.matched && result.score > 0;
                        }()) {
                        score += result.score;
                        if (string.startsWith(queryItem, Qt::CaseInsensitive)) {
                            score *= 2;
                        }
                    }
                }
            }
            return score;
        };

        auto makeScoreFromList = [&makeScore](const QStringList &strings) {
            qreal score = 0;
            uint matched = 1;
            for (const auto &string : strings) {
                score += makeScore(string);
                matched += 1;
            }
            return score / matched; // Average the score. If we have 10 keywords matching we'd get a huge score for no reason
        };

        // Note for the future: we could multiply the scores by decreasing values from 1.0, 0.9, … but be mindful that this complicates the scoring logic
        // and, more importantly, skews the results. A poor name match would easily outscore a good keyword match.
        const std::map<qreal, KRunner::QueryMatch::CategoryRelevance> confidences = {
            {makeScore(name), KRunner::QueryMatch::CategoryRelevance::High},
            {makeScore(service->untranslatedName()), KRunner::QueryMatch::CategoryRelevance::High},
            {makeScore(service->genericName()), KRunner::QueryMatch::CategoryRelevance::Moderate},
            {makeScoreFromList(service->keywords()), KRunner::QueryMatch::CategoryRelevance::Moderate},
            // FIXME: drop this since we seem to be leaning away from comments
            // {makeScore(service->comment()), KRunner::QueryMatch::CategoryRelevance::Low},
        };

        const auto finalScore = std::accumulate(confidences.begin(), confidences.end(), 0, [](const auto &acc, const auto &pair) {
            return acc + pair.first;
        });

        if (finalScore <= 0) {
            qCDebug(RUNNER_SERVICES) << "No score for" << name << "with query" << query;
            return std::nullopt; // No score, no match.
        }

        qDebug() << "Final score for" << name << "is" << finalScore << "with category relevance" << qToUnderlying(confidences.rbegin()->second);
        return Score{.value = finalScore, .categoryRelevance = confidences.rbegin()->second};
    }

    void matchNameKeywordAndGenericName()
    {
        for (const KService::Ptr &service : m_services) {
            if (disqualify(service)) {
                continue;
            }

            KRunner::QueryMatch match(m_runner);
            auto score = fuzzyScore(service, match);
            if (!score) {
                continue;
            }

            setupMatch(service, match);
            match.setCategoryRelevance(score->categoryRelevance);
            match.setRelevance(score->value);
            qCDebug(RUNNER_SERVICES) << match.text() << "is this relevant:" << match.relevance() << "category relevance" << match.categoryRelevance();

            matches << match;
        }
    }

    void matchCategories()
    {
        // Do not match categories for short queries, BUG: 469769
        if (weightedTermLength < 5) {
            return;
        }
        for (const KService::Ptr &service : m_services) {
            const QStringList categories = service->categories();
            if (disqualify(service) || !contains(categories, queryList)) {
                continue;
            }
            qCDebug(RUNNER_SERVICES) << service->name() << "is an exact match!" << service->storageId() << service->exec();

            KRunner::QueryMatch match(m_runner);
            setupMatch(service, match);

            qreal relevance = 0.4;
            if (std::ranges::any_of(categories, [this](const QString &category) {
                    return category.compare(query, Qt::CaseInsensitive) == 0;
                })) {
                relevance = 0.6;
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
        for (const KService::Ptr &service : m_services) {
            const auto actions = service->actions();
            // Skip SystemSettings as we find KCMs already
            if (actions.isEmpty() || service->storageId() == QLatin1String("systemsettings.desktop")) {
                continue;
            }

            for (const KServiceAction &action : actions) {
                if (action.text().isEmpty() || hasSeen(action)) {
                    continue;
                }
                seen(action);

                const auto matchIndex = action.text().indexOf(query, 0, Qt::CaseInsensitive);
                if (matchIndex < 0) {
                    continue;
                }

                KRunner::QueryMatch match(m_runner);
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
                    match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::High); // Give it a higer match type to ensure it is shown, BUG: 455436
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

    QList<KRunner::QueryMatch> matches;
    QString query;
    QList<QStringView> queryList;
    int weightedTermLength = -1;
};

ServiceRunner::ServiceRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    addSyntax(QStringLiteral(":q:"), i18n("Finds applications whose name or description match :q:"));

    connect(this, &KRunner::AbstractRunner::prepare, this, [this]() {
        m_matching = true;
        if (m_services.isEmpty()) {
            m_services = KApplicationTrader::query([](const KService::Ptr &service) {
                return !service->noDisplay();
            });
        } else {
            KSycoca::self()->ensureCacheValid();
        }
    });
    connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
        m_matching = false;
    });
}

void ServiceRunner::init()
{
    Init("default"_L1);
    //  connect to the thread-local singleton here
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, [this]() {
        if (m_matching) {
            m_services = KApplicationTrader::query([](const KService::Ptr &service) {
                return !service->noDisplay();
            });
        } else {
            // Invalidate for the next match session
            m_services.clear();
        }
    });
}

void ServiceRunner::match(KRunner::RunnerContext &context)
{
    ServiceFinder finder(this, m_services);
    finder.match(context);
}

void ServiceRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const QUrl dataUrl = match.data().toUrl();

    KService::Ptr service = KService::serviceByStorageId(dataUrl.path());
    if (!service) {
        return;
    }

    KActivities::ResourceInstance::notifyAccessed(QUrl(QString(u"applications:" + service->storageId())), QStringLiteral("org.kde.krunner"));

    KIO::ApplicationLauncherJob *job = nullptr;

    const QString actionName = QUrlQuery(dataUrl).queryItemValue(QStringLiteral("action"));
    if (actionName.isEmpty()) {
        job = new KIO::ApplicationLauncherJob(service);
    } else {
        const auto actions = service->actions();
        auto it = std::ranges::find_if(actions, [&actionName](const KServiceAction &action) {
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

#include "moc_servicerunner.cpp"
