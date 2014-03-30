/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
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

#include "browserfactory.h"
#include "browser.h"
#include "browsers/kdebrowser.h"
#include "browsers/firefox.h"
#include "browsers/opera.h"
#include "browsers/chromefindprofile.h"
#include "browsers/chrome.h"

Browser *BrowserFactory::find(const QString& browserName, QObject* parent)
{
    if(m_previousBrowserName == browserName) {
      return m_previousBrowser;
    }
    delete m_previousBrowser;
    m_previousBrowserName = browserName;
    if (browserName.contains("firefox", Qt::CaseInsensitive) || browserName.contains("iceweasel", Qt::CaseInsensitive)) {
        m_previousBrowser = new Firefox(parent);
    } else if (browserName.contains("opera", Qt::CaseInsensitive)) {
        m_previousBrowser = new Opera(parent);
    } else if (browserName.contains("chrome", Qt::CaseInsensitive)) {
        m_previousBrowser = new Chrome(new FindChromeProfile("google-chrome", QDir::homePath(), parent), parent);
    } else if (browserName.contains("chromium", Qt::CaseInsensitive)) {
        m_previousBrowser = new Chrome(new FindChromeProfile("chromium", QDir::homePath(), parent), parent);
    } else {
        m_previousBrowser = new KDEBrowser(parent);
    }

    return m_previousBrowser;
}


BrowserFactory::BrowserFactory(QObject *parent)
    : QObject(parent), m_previousBrowser(0), m_previousBrowserName("invalid")
{
}

