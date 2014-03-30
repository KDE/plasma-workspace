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

#include "webviewoverlay.h"

#include <QApplication>
#include <QGraphicsLinearLayout>
#include <QStyleOptionGraphicsItem>
#include <QPainter>

#include <Plasma/PushButton>
#include <Plasma/WebView>

#include "webbrowser.h"
#include "webbrowserpage.h"

WebViewOverlay::WebViewOverlay(WebBrowser *parent)
    : QGraphicsWidget(parent)
{
      QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(this);
      layout->setOrientation(Qt::Vertical);
      
      m_webView = new Plasma::WebView(this);
      m_webView->setPage(new WebBrowserPage(parent));
      m_webView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      layout->addItem(m_webView);
      
      m_closeButton = new Plasma::PushButton(this);
      m_closeButton->setText(i18n("Close"));
      connect(m_closeButton, SIGNAL(clicked()), this, SIGNAL(closeRequested()));
      layout->addItem(m_closeButton);
      connect(m_webView->page(), SIGNAL(windowCloseRequested()), this, SIGNAL(closeRequested()));
}

QWebPage *WebViewOverlay::page()
{
    return m_webView->page();
}

void WebViewOverlay::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)
  
    painter->fillRect(option->rect, QApplication::palette().window());
}

#include "webviewoverlay.moc"
