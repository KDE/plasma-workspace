/*
    SPDX-FileCopyrightText: 2007 Teemu Rytilahti <tpr@iki.fi>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "webshortcutrunner.h"

#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/CommandLauncherJob>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KRunner/RunnerManager>
#include <KSharedConfig>
#include <KShell>
#include <KSycoca>
#include <KUriFilter>
#include <QDBusConnection>
#include <QIcon>
#include <defaultservice.h>

using namespace Qt::StringLiterals;

WebshortcutRunner::WebshortcutRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_match(this)
    , m_filterBeforeRun(false)
{
    m_match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
    m_match.setRelevance(0.9);

    // Listen for KUriFilter plugin config changes and update state...
    QDBusConnection sessionDbus = QDBusConnection::sessionBus();
    sessionDbus.connect(QString(), QStringLiteral("/"), QStringLiteral("org.kde.KUriFilterPlugin"), QStringLiteral("configure"), this, SLOT(loadSyntaxes()));
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &WebshortcutRunner::configurePrivateBrowsingActions);
    setMinLetterCount(3);

    connect(qobject_cast<KRunner::RunnerManager *>(parent), &KRunner::RunnerManager::queryFinished, this, [this]() {
        if (m_lastUsedContext.isValid() && !m_defaultKey.isEmpty() && m_lastUsedContext.matches().isEmpty()) {
            const QString queryWithDefaultProvider = m_defaultKey + m_delimiter + m_lastUsedContext.query();
            KUriFilterData filterData(queryWithDefaultProvider);
            if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter)) {
                m_match.setText(i18n("Search %1 for %2", filterData.searchProvider(), filterData.searchTerm()));
                m_match.setData(filterData.uri());
                m_match.setIconName(filterData.iconName());
                m_lastUsedContext.addMatch(m_match);
            }
        }
    });
}

void WebshortcutRunner::loadSyntaxes()
{
    KUriFilterData filterData(QStringLiteral(":q"));
    filterData.setSearchFilteringOptions(KUriFilterData::RetrieveAvailableSearchProvidersOnly);
    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter)) {
        m_delimiter = filterData.searchTermSeparator();
    }
    m_regex = QRegularExpression(QStringLiteral("^([^ ]+)%1").arg(QRegularExpression::escape(m_delimiter)));

    QList<KRunner::RunnerSyntax> syns;
    const QStringList providers = filterData.preferredSearchProviders();
    const static QRegularExpression replaceRegex(QStringLiteral(":q$"));
    const QString placeholder = QStringLiteral(":q:");
    for (const QString &provider : providers) {
        KRunner::RunnerSyntax s(filterData.queryForPreferredSearchProvider(provider).replace(replaceRegex, placeholder),
                                i18n("Opens \"%1\" in a web browser with the query :q:.", provider));
        syns << s;
    }
    if (!providers.isEmpty()) {
        QString defaultKey = filterData.queryForSearchProvider(providers.constFirst()).defaultKey();
        KRunner::RunnerSyntax s(QStringLiteral("!%1 :q:").arg(defaultKey), i18n("Search using the DuckDuckGo bang syntax"));
        syns << s;
    }

    setSyntaxes(syns);
    m_lastFailedKey.clear();
    m_lastProvider.clear();
    m_lastKey.clear();

    // When we reload the syntaxes, our WebShortcut config has changed or is initialized
    const KConfigGroup grp = KSharedConfig::openConfig(u"kuriikwsfilterrc"_s)->group(u"General"_s);
    m_defaultKey = grp.readEntry("DefaultWebShortcut", QStringLiteral("duckduckgo"));
}

void WebshortcutRunner::configurePrivateBrowsingActions()
{
    m_match.setActions({});

    auto service = DefaultService::browser();
    if (!service) {
        return;
    }
    const auto actions = service->actions();
    for (const auto &action : actions) {
        bool containsPrivate = action.text().contains(QLatin1String("private"), Qt::CaseInsensitive);
        bool containsIncognito = action.text().contains(QLatin1String("incognito"), Qt::CaseInsensitive);
        if (containsPrivate || containsIncognito) {
            m_privateAction = action;
            const QString text = containsPrivate ? i18n("Search in private window") : i18n("Search in incognito window");
            m_match.setActions({KRunner::Action(action.exec(), m_iconName, text)});
            return;
        }
    }
}

void WebshortcutRunner::match(KRunner::RunnerContext &context)
{
    m_lastUsedContext = context;
    const QString term = context.query();
    const static QRegularExpression bangRegex(QStringLiteral("!([^ ]+).*"));
    const auto bangMatch = bangRegex.match(term);
    QString key;
    QString rawQuery = term;

    if (bangMatch.hasMatch()) {
        key = bangMatch.captured(1);
        rawQuery = rawQuery.remove(rawQuery.indexOf(key) - 1, key.size() + 1);
    } else {
        const auto normalMatch = m_regex.match(term);
        if (normalMatch.hasMatch()) {
            key = normalMatch.captured(0);
            rawQuery = rawQuery.mid(key.length());
        }
    }
    if (key.isEmpty() || key == m_lastFailedKey) {
        return; // we already know it's going to suck ;)
    }

    // Do a fake user feedback text update if the keyword has not changed.
    // There is no point filtering the request on every key stroke.
    // filtering
    if (m_lastKey == key) {
        m_filterBeforeRun = true;
        m_match.setText(i18n("Search %1 for %2", m_lastProvider, rawQuery));
        context.addMatch(m_match);
        return;
    }

    KUriFilterData filterData(term);
    if (!KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter)) {
        m_lastFailedKey = key;
        return;
    }

    // Reuse key/provider for next matches. Other variables ca be reused, because the same match object is used
    m_lastKey = key;
    m_lastProvider = filterData.searchProvider();
    m_match.setIconName(filterData.iconName());
    m_match.setId(QString(u"WebShortcut:" + key));

    m_match.setText(i18n("Search %1 for %2", m_lastProvider, filterData.searchTerm()));
    m_match.setData(filterData.uri());
    m_match.setUrls(QList<QUrl>{filterData.uri()});
    context.addMatch(m_match);
}

void WebshortcutRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    QUrl location;
    if (m_filterBeforeRun) {
        m_filterBeforeRun = false;
        KUriFilterData filterData(context.query());
        if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::WebShortcutFilter))
            location = filterData.uri();
    } else {
        location = match.data().toUrl();
    }

    if (!location.isEmpty()) {
        if (match.selectedAction()) {
            QString command;

            // Chrome's exec line does not have a URL placeholder
            // Firefox's does, but only sometimes, depending on the distro
            // Replace placeholders if found, otherwise append at the end
            if (m_privateAction.exec().contains(u"%u")) {
                command = m_privateAction.exec().replace(u"%u"_s, KShell::quoteArg(location.toString()));
            } else if (m_privateAction.exec().contains(u"%U")) {
                command = m_privateAction.exec().replace(u"%U"_s, KShell::quoteArg(location.toString()));
            } else {
                command = m_privateAction.exec() + QLatin1Char(' ') + KShell::quoteArg(location.toString());
            }

            auto *job = new KIO::CommandLauncherJob(command);
            job->start();
        } else {
            auto job = new KIO::OpenUrlJob(location);
            job->start();
        }
    }
}

void WebshortcutRunner::init()
{
    m_iconName = QIcon::fromTheme(QStringLiteral("view-private"), QIcon::fromTheme(QStringLiteral("view-hidden"))).name();
    configurePrivateBrowsingActions();
    loadSyntaxes();
}

K_PLUGIN_CLASS_WITH_JSON(WebshortcutRunner, "plasma-runner-webshortcuts.json")

#include "webshortcutrunner.moc"

#include "moc_webshortcutrunner.cpp"
