/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserfactory.h"
#include "browsers/browser.h"
#include "browsers/chrome.h"
#include "browsers/chromefindprofile.h"
#include "browsers/falkon.h"
#include "browsers/firefox.h"
#include "browsers/konqueror.h"
#include "browsers/opera.h"

Browser *BrowserFactory::find(const QString &browserName, QObject *parent)
{
    if (m_previousBrowserName == browserName) {
        return m_previousBrowser;
    }
    delete m_previousBrowser;
    m_previousBrowserName = browserName;
    if (browserName.contains(QLatin1String("firefox"), Qt::CaseInsensitive) || browserName.contains(QLatin1String("iceweasel"), Qt::CaseInsensitive)) {
        m_previousBrowser = new Firefox(QDir::homePath() + QStringLiteral("/.mozilla/firefox"), parent);
    } else if (browserName.contains(QLatin1String("opera"), Qt::CaseInsensitive)) {
        m_previousBrowser = new Opera(parent);
    } else if (browserName.contains(QLatin1String("chrome"), Qt::CaseInsensitive)) {
        m_previousBrowser = new Chrome(new FindChromeProfile(QStringLiteral("google-chrome"), QDir::homePath(), parent), parent);
    } else if (browserName.contains(QLatin1String("chromium"), Qt::CaseInsensitive)) {
        m_previousBrowser = new Chrome(new FindChromeProfile(QStringLiteral("chromium"), QDir::homePath(), parent), parent);
    } else if (browserName.contains(QLatin1String("falkon"), Qt::CaseInsensitive)) {
        m_previousBrowser = new Falkon(parent);
    } else {
        m_previousBrowser = new Konqueror(parent);
    }

    return m_previousBrowser;
}

BrowserFactory::BrowserFactory(QObject *parent)
    : QObject(parent)
    , m_previousBrowser(nullptr)
    , m_previousBrowserName(QStringLiteral("invalid"))
{
}
