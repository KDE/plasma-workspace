/***************************************************************************
 *   Copyright (C) 2008 by Marco Martin <notmart@gmail.com>                *
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

#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QHash>
#include <QWebPage>

#include <Plasma/PopupApplet>
#include <Plasma/DataEngine>

#include "ui_webbrowserconfig.h"
#include "browsermessagebox.h"

class WebViewOverlay;

class QGraphicsLinearLayout;
class QStandardItemModel;
class QStandardItem;
class QTimer;
class KUrlPixmapProvider;
class KHistoryComboBox;
class KUrl;
class KCompletion;
class KBookmarkManager;
class KBookmarkGroup;
class QModelIndex;
class QAction;
class BookmarksDelegate;
class BookmarkItem;

namespace Plasma
{
    class BrowserHistoryComboBox;
    class IconWidget;
    class ComboBox;
    class WebView;
    class TreeView;
    class Slider;
}


using Plasma::MessageButton;

class WebBrowser : public Plasma::PopupApplet
{
    Q_OBJECT
public:
    WebBrowser(QObject *parent, const QVariantList &args);
    ~WebBrowser();

    QGraphicsWidget *graphicsWidget();
    void paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect);
    QWebPage *createWindow(QWebPage::WebWindowType type);
    
    //TODO: put in a separate file
    enum BookmarkRoles
    {
        UrlRole = Qt::UserRole+1,
        BookmarkRole = Qt::UserRole+2
    };

public Q_SLOTS:
    void dataUpdated(const QString &source, const Plasma::DataEngine::Data &data);
    void saveFormDataRequested(const QString &uid, const QUrl &url);
    
protected:
    void saveState(KConfigGroup &cg) const;
    Plasma::IconWidget *addTool(const QString &iconString, QGraphicsLinearLayout *layout);
    void createConfigurationInterface(KConfigDialog *parent);
    void constraintsEvent(Plasma::Constraints constraints);
    
protected Q_SLOTS:
    void back();
    void forward();
    void reload();
    void returnPressed();
    void urlChanged(const QUrl &url);
    void comboTextChanged(const QString &string);
    void addBookmark();
    void removeBookmark(const QModelIndex &index);
    void removeBookmark();
    void bookmarksToggle();
    void bookmarkClicked(const QModelIndex &index);
    void zoom(int value);
    void loadProgress(int progress);
    void bookmarksModelInit();
    void configAccepted();
    void configChanged();
    void bookmarksAnimationFinished();
    void removeBookmarkMessageButtonPressed(const MessageButton button);
    void closeWebViewOverlay();

    void acceptWalletRequest();
    void rejectWalletRequest();

private:
    void fillGroup(BookmarkItem *parentItem, const KBookmarkGroup &group);
    void updateOverlaysGeometry();
    QHash<BrowserMessageBox *, QString> walletRequests;
    

    QGraphicsLinearLayout *m_layout;
    QGraphicsLinearLayout *m_toolbarLayout;
    QGraphicsLinearLayout *m_statusbarLayout;
    Plasma::WebView *m_browser;
    WebViewOverlay *m_webOverlay;
    KUrl m_url;
    int m_verticalScrollValue;
    int m_horizontalScrollValue;
    KUrlPixmapProvider *m_pixmapProvider;
    KCompletion *m_completion;
    KBookmarkManager *m_bookmarkManager;
    QStandardItemModel *m_bookmarkModel;
    Plasma::TreeView *m_bookmarksView;
    Plasma::Animation *m_bookmarksViewAnimation;
    
    QTimer *m_autoRefreshTimer;
    bool m_autoRefresh;
    int m_autoRefreshInterval;

    QGraphicsWidget *m_graphicsWidget;

    Plasma::BrowserHistoryComboBox *m_historyCombo;
    KHistoryComboBox *m_nativeHistoryCombo;
    BookmarksDelegate *m_bookmarksDelegate;

    Plasma::IconWidget *m_back;
    Plasma::IconWidget *m_forward;

    Plasma::IconWidget *m_go;
    QAction *m_goAction;
    QAction *m_reloadAction;

    Plasma::IconWidget *m_addBookmark;
    QAction *m_addBookmarkAction;
    QAction *m_removeBookmarkAction;

    Plasma::IconWidget *m_organizeBookmarks;
    Plasma::IconWidget *m_stop;
    Plasma::Slider *m_zoom;

    Ui::WebBrowserConfig ui;
};

#endif
