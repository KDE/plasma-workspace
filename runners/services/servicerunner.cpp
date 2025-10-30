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

#include "bitap.h"
#include "debug.h"
#include "levenshtein.h"

using namespace Qt::StringLiterals;

namespace
{

struct Score {
    qreal value = 0.0; // The final score, it is the sum of all scores.
    KRunner::QueryMatch::CategoryRelevance categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Lowest; // The category relevance of the match.
};

struct ScoreCard {
    QStringView search;
    QString term;
    Bitap::Match bitap;
    qreal bitapScore;
    int levenshtein;
    qreal levenshteinScore;
    bool perfectMatch;
    bool completeMatch;
    bool startsWith;
};

QDebug operator<<(QDebug dbg, const ScoreCard &card)
{
    dbg.nospace() << "Scorecard(" << "search: " << card.search << ", term: " << card.term << "bitap: " << card.bitap << ", bitapScore: " << card.bitapScore
                  << ", levenshtein: " << card.levenshtein << ", levenshteinScore: " << card.levenshteinScore << ", perfectMatch: " << card.perfectMatch
                  << ", completeMatch: " << card.completeMatch << ", startsWith: " << card.startsWith << ")";
    return dbg;
}

using ScoreCards = std::vector<ScoreCard>;

struct WeightedScoreCard {
    ScoreCards cards;
    qreal weight;
};

QDebug operator<<(QDebug dbg, const WeightedScoreCard &card)
{

    dbg.nospace() << "WeightedCard[";
    for (const auto &scoreCard : card.cards) {
        dbg.nospace() << scoreCard;
        if (&scoreCard != &card.cards.back()) {
            dbg.nospace() << ", ";
        }
    }
    dbg.nospace() << "]";
    return dbg;
}

auto makeScores(const auto &notNormalizedString, const auto &queryList) {
    if (notNormalizedString.isEmpty()) {
        return ScoreCards{}; // No string, no score.
    }

    const auto string = notNormalizedString.toLower();

    ScoreCards cards;
    for (const auto &queryItem : queryList) {
        constexpr auto maxDistance = 1;
        const auto bitap = Bitap::bitap(string, queryItem, maxDistance);
        if (!bitap) {
            // One of the query items didn't match. This means the entire query is not a match
            return ScoreCards{};
        }

        const auto bitapScore = Bitap::score(string, bitap.value(), maxDistance);

        // Mind that we give different levels of bonus. This is important to imply ordering within competing matches of the same "type".
        // If we perfectly match that gives a bonus for not requiring any changes.
        const auto perfectMatch = bitap.value().distance == 0;
        // If we match the entire length of the string that gets a bonus (disregarding distance, that was considered above).
        const auto completeMatch = bitap->size >= (queryItem.size());
        // If the string starts with the query item that gets a bonus.
        const auto startsWith = (string.startsWith(queryItem, Qt::CaseInsensitive));

        // Also consider the distance between the input and the query item.
        // If one is "yolotrollingservice" and the other is "yolo" then we must consider them worse matches than say "yolotroll".
        const auto levenshtein = Levenshtein::distance(string, queryItem);

        cards.emplace_back(ScoreCard{
            .search = queryItem,
            .term = string,
            .bitap = *bitap,
            .bitapScore = bitapScore + (perfectMatch ? 4.0 : 0.0) + (completeMatch ? 3.0 : 0.0) + (startsWith ? 2.0 : 0.0),
            .levenshtein = levenshtein,
            .levenshteinScore = Levenshtein::score(string, levenshtein),
            .perfectMatch = perfectMatch,
            .completeMatch = completeMatch,
            .startsWith = startsWith,
        });
    }

    return cards;
};


auto makeScoreFromList(const auto &queryList, const QStringList &strings) {
    // This turns the loop inside out. For every query item we must find a match in our keywords or we discard
    ScoreCards cards;
    // e.g. text,editor,programming
    for (const auto &queryItem : queryList) {
        // e.g. text;txt;editor;programming;programmer;development;developer;code;
        auto found = false;
        ScoreCards queryCards;
        for (const auto &string : strings) {
            auto stringCards = makeScores(string, QList{queryItem});
            if (stringCards.empty()) {
                continue; // The combination didn't match.
            }
            for (auto &scoreCard : stringCards) {
                if (scoreCard.levenshteinScore < 0.8) {
                    continue; // Not a good match, skip it. We are very strict with keywords
                }
                found = true;
                queryCards.push_back(scoreCard);
            }
            // We do not break because other string might also match, improving the score.
        }
        if (!found) {
            // No item in strings matched the query item. This means the entire query is not a match.
            return ScoreCards{};
        }
#ifdef __cpp_lib_containers_ranges
        cards.append_range(queryCards);
#else
        cards.insert(cards.end(), queryCards.cbegin(), queryCards.cend());
#endif
    }
    return cards;
};

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
        query = context.query().toLower();
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

        match.setText(name);

        QUrl url(service->storageId());
        url.setScheme(QStringLiteral("applications"));
        match.setData(url);
        match.setUrls({QUrl::fromLocalFile(service->entryPath())});

        QString urlPath = resolvedArgs(service);
        if (urlPath.isEmpty()) {
            // Otherwise we might filter out broken services. Rather than hiding them, it is better to show an error message on launch (as done by KIO's jobs)
            urlPath = service->exec();
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

    QString resolvedArgs(const KService::Ptr &service)
    {
        KIO::DesktopExecParser parser(*service, {});
        QStringList resultingArgs = parser.resultingArguments();
        if (const auto error = parser.errorMessage(); resultingArgs.isEmpty() && !error.isEmpty()) {
            qCWarning(RUNNER_SERVICES) << "Failed to resolve executable from service. Error:" << error;
            return QString();
        }

        // Remove any environment variables.
        if (KIO::DesktopExecParser::executableName(service->exec()) == QLatin1String("env")) {
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

    [[nodiscard]] std::optional<Score> fuzzyScore(KService::Ptr service)
    {
        if (queryList.isEmpty()) {
            return std::nullopt; // No query, no score.
        }

        const auto name = service->name();
        if (name.compare(query, Qt::CaseInsensitive) == 0) {
            // Absolute match. Can't get any better than this.
            return Score{.value = std::numeric_limits<decltype(Score::value)>::max(), .categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Highest};
        }

        const auto weightedCards = {
            WeightedScoreCard{.cards = makeScores(name, queryList), .weight = 100.0},
            WeightedScoreCard{.cards = makeScores(service->untranslatedName(), queryList), .weight = 75.0},
            WeightedScoreCard{.cards = makeScores(service->genericName(), queryList), .weight = 50.0},
            WeightedScoreCard{.cards = makeScoreFromList(queryList, service->keywords()), .weight = 25.0},
        };

        if (RUNNER_SERVICES().isDebugEnabled()) {
            qCDebug(RUNNER_SERVICES) << "+++++++ Weighted Cards for" << name;
            for (const auto &weightedCard : weightedCards) {
                qCDebug(RUNNER_SERVICES) << weightedCard;
            }
            qCDebug(RUNNER_SERVICES) << "-------";
        }

        int scores = 1; // starts at 1 to avoid division by zero. Is the number of scores to average over.
        qreal finalScore = 0.0;

        const auto perfectMatchScore = [&](const auto &weightedCards) {
            if (std::ranges::any_of(weightedCards.cards, [](const ScoreCard &card) {
                    return card.perfectMatch;
                })) {
                finalScore += 100.0 * weightedCards.weight;
                scores++;
            }
        };

        const auto fuzzyScore = [&](const auto &weightedCards) {
            qreal weightedScore = 0.0;
            for (const auto &scoreCard : weightedCards.cards) {
                weightedScore += (scoreCard.bitapScore + scoreCard.levenshteinScore) * weightedCards.weight;
                scores++;
            }
            finalScore += weightedScore;
        };

        for (const auto &weightedCard : weightedCards) {
            perfectMatchScore(weightedCard);
            fuzzyScore(weightedCard);
        }

        finalScore = finalScore / scores; // Average the score for this card

        qCDebug(RUNNER_SERVICES) << "Final score for" << name << "is" << finalScore;
        if (finalScore > 0.0) {
            return Score{.value = finalScore, .categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Moderate};
        }

        return std::nullopt;
    }

    void matchNameKeywordAndGenericName()
    {
        static auto isTest = QStandardPaths::isTestModeEnabled();

        for (const KService::Ptr &service : m_services) {
            if (isTest && !service->name().contains("ServiceRunnerTest"_L1)) {
                continue; // Skip services that are not part of the test.
            }

            KRunner::QueryMatch match(m_runner);
            auto score = fuzzyScore(service);
            if (!score || disqualify(service)) {
                continue;
            }

            setupMatch(service, match);
            match.setCategoryRelevance(score->categoryRelevance);
            // KRunner may apply counter-productive bumps to the score of up to 0.5 points. That can easily produce
            // unrealistic results where suddenly one thing is top score for no discernable reason. We outscore KRunner
            // by moving our score range into the hundreds, making the 0.5 bump negligible.
            // In Plasma 6.6 and later we'll depend on a KRunner that does no longer have this behavior.
            constexpr qreal outscoreMultiplier = 100.0;
            match.setRelevance(score->value * outscoreMultiplier);
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
