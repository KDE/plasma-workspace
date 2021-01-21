/*
 *   Copyright (C) 2007 Teemu Rytilahti <tpr@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "webshortcutrunner.h"

#include <KApplicationTrader>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KShell>
#include <KSycoca>
#include <KUriFilter>
#include <QDBusConnection>
#include <QDesktopServices>

WebshortcutRunner::WebshortcutRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
    , m_match(this)
    , m_filterBeforeRun(false)
{
    setObjectName(QStringLiteral("Web Shortcut"));
    m_match.setType(Plasma::QueryMatch::ExactMatch);
    m_match.setRelevance(0.9);

    // Listen for KUriFilter plugin config changes and update state...
    QDBusConnection sessionDbus = QDBusConnection::sessionBus();
    sessionDbus.connect(QString(), QStringLiteral("/"), QStringLiteral("org.kde.KUriFilterPlugin"), QStringLiteral("configure"), this, SLOT(loadSyntaxes()));
    loadSyntaxes();
    configurePrivateBrowsingActions();
    connect(KSycoca::self(), QOverload<>::of(&KSycoca::databaseChanged), this, &WebshortcutRunner::configurePrivateBrowsingActions);
    setMinLetterCount(3);
}

WebshortcutRunner::~WebshortcutRunner()
{
}

void WebshortcutRunner::loadSyntaxes()
{
    KUriFilterData filterData(QStringLiteral(":q"));
    filterData.setSearchFilteringOptions(KUriFilterData::RetrieveAvailableSearchProvidersOnly);
    if (KUriFilter::self()->filterSearchUri(filterData, KUriFilter::NormalTextFilter)) {
        m_delimiter = filterData.searchTermSeparator();
    }

    QList<Plasma::RunnerSyntax> syns;
    const QStringList providers = filterData.preferredSearchProviders();
    for (const QString &provider : providers) {
        Plasma::RunnerSyntax s(filterData.queryForPreferredSearchProvider(provider), /*":q:",*/
                               i18n("Opens \"%1\" in a web browser with the query :q:.", provider));
        syns << s;
    }

    setSyntaxes(syns);
    m_lastFailedKey.clear();
    m_lastProvider.clear();
    m_lastKey.clear();
}

void WebshortcutRunner::configurePrivateBrowsingActions()
{
    clearActions();
    const QString browserFile = KSharedConfig::openConfig(QStringLiteral("kdeglobals"))->group("General").readEntry("BrowserApplication");
    KService::Ptr service;
    if (!browserFile.isEmpty()) {
        service = KService::serviceByStorageId(browserFile);
    }
    if (!service) {
        service = KApplicationTrader::preferredService(QStringLiteral("text/html"));
    }
    if (!service) {
        return;
    }
    const auto actions = service->actions();
    for (const auto &action : actions) {
        bool containsPrivate = action.text().contains(QLatin1String("private"), Qt::CaseInsensitive);
        bool containsIncognito = action.text().contains(QLatin1String("incognito"), Qt::CaseInsensitive);
        if (containsPrivate || containsIncognito) {
            m_privateAction = action;
            const QString actionText = containsPrivate ? i18n("Search in private window") : i18n("Search in incognito window");
            const QIcon icon = QIcon::fromTheme(QStringLiteral("view-private"), QIcon::fromTheme(QStringLiteral("view-hidden")));
            m_match.setActions({addAction(QStringLiteral("privateSearch"), icon, actionText)});
            return;
        }
    }
}

void WebshortcutRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    const static QRegularExpression bangRegex(QStringLiteral("!([^ ]+).*"));
    const static QRegularExpression normalRegex(QStringLiteral("^([^ ]+)%1").arg(QRegularExpression::escape(m_delimiter)));
    const auto bangMatch = bangRegex.match(term);
    QString key;
    QString rawQuery = term;

    if (bangMatch.hasMatch()) {
        key = bangMatch.captured(1);
        rawQuery = rawQuery.remove(rawQuery.indexOf(key) - 1, key.size() + 1);
    } else {
        const auto normalMatch = normalRegex.match(term);
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
    m_match.setId(QStringLiteral("WebShortcut:") + key);

    m_match.setText(i18n("Search %1 for %2", m_lastProvider, filterData.searchTerm()));
    m_match.setData(filterData.uri());
    context.addMatch(m_match);
}

void WebshortcutRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
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
            const auto command = m_privateAction.exec() + QLatin1Char(' ') + KShell::quoteArg(location.toString());
            auto *job = new KIO::CommandLauncherJob(command);
            job->start();
        } else {
            QDesktopServices::openUrl(location);
        }
    }
}

K_EXPORT_PLASMA_RUNNER_WITH_JSON(WebshortcutRunner, "plasma-runner-webshortcuts.json")

#include "webshortcutrunner.moc"
