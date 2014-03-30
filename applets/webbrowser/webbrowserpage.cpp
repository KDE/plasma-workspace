/***************************************************************************
 *   Copyright (C) 2010 Davide Bettio <davide.bettio@kdemail.net>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "webbrowserpage.h"
#include "webbrowser.h"
#include "errorpage.h"

#include <QNetworkReply>
#include <QWebFrame>

#include <kwebwallet.h>

WebBrowserPage::WebBrowserPage(WebBrowser *parent)
          : KWebPage(parent)
{
    m_browser = parent;
    //settings()->setAttribute(QWebSettings::JavaEnabled, true);
    settings()->setAttribute(QWebSettings::PluginsEnabled, true);

    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply*)), this, SLOT(networkAccessFinished(QNetworkReply*)));
    connect(this, SIGNAL(loadFinished(bool)), this, SLOT(pageLoadFinished(bool)));
    connect(wallet(), SIGNAL(saveFormDataRequested(QString,QUrl)),
            m_browser, SLOT(saveFormDataRequested(QString,QUrl)));
}
  
QWebPage *WebBrowserPage::createWindow(WebWindowType type)
{
    return m_browser->createWindow(type);
}

void WebBrowserPage::pageLoadFinished(bool ok)
{
    if (ok){
        wallet()->fillFormData(mainFrame());
    }
}

void WebBrowserPage::networkAccessFinished(QNetworkReply *nReply)
{
    switch (nReply->error()){
        case QNetworkReply::NoError:
        case QNetworkReply::UnknownContentError:
        case QNetworkReply::ContentNotFoundError:
           return;

        default:
           mainFrame()->setHtml(errorPageHtml(webKitErrorToKIOError(nReply->error()), nReply->url().toString(), nReply->url()));
    }
}

#include "webbrowserpage.moc"
