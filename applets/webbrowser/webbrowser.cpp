/***************************************************************************
 *   Copyright (C) 2008 Marco Martin <notmart@gmail.com>                   *
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

#include "webbrowser.h"

#include <QGraphicsLinearLayout>
#include <QModelIndex>
#include <QNetworkReply>
#include <QPainter>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>
#include <QWebPage>
#include <QWebFrame>
#include <QWebHistory>

#include <KIcon>
#include <KCompletion>
#include <KBookmarkManager>
#include <KIconLoader>
#include <KUrlPixmapProvider>
#include <KUriFilter>
#include <KMessageBox>
#include <KConfigDialog>
#include <KHistoryComboBox>
#include <KWebPage>
#include <kwebwallet.h>
#include <KStandardDirs>

#include <Plasma/Animation>
#include <Plasma/IconWidget>
#include <Plasma/WebView>
#include <Plasma/TreeView>
#include <Plasma/PushButton>
#include <Plasma/Slider>

#include "bookmarksdelegate.h"
#include "bookmarkitem.h"
#include "webviewoverlay.h"
#include "browserhistorycombobox.h"
#include "browsermessagebox.h"
#include "errorpage.h"
#include "webbrowserpage.h"

using Plasma::MessageButton;

WebBrowser::WebBrowser(QObject *parent, const QVariantList &args)
        : Plasma::PopupApplet(parent, args),
          m_browser(0),
          m_verticalScrollValue(0),
          m_horizontalScrollValue(0),
          m_completion(0),
          m_bookmarkManager(0),
          m_bookmarkModel(0),
          m_autoRefreshTimer(0)
{
    setHasConfigurationInterface(true);
    setAspectRatioMode(Plasma::IgnoreAspectRatio);

    m_historyCombo = 0;
    m_graphicsWidget = 0;
    m_webOverlay = 0;

    m_layout = 0;
    resize(500,500);
    if (!args.isEmpty()) {
        m_url = KUrl(args.value(0).toString());
    }
    setPopupIcon("konqueror");
}

QGraphicsWidget *WebBrowser::graphicsWidget()
{
    if (m_graphicsWidget) {
        return m_graphicsWidget;
    }

    m_layout = new QGraphicsLinearLayout(Qt::Vertical);
    m_toolbarLayout = new QGraphicsLinearLayout(Qt::Horizontal);
    m_statusbarLayout = new QGraphicsLinearLayout(Qt::Horizontal);

    m_back = addTool("go-previous", m_toolbarLayout);
    m_forward = addTool("go-next", m_toolbarLayout);

    m_nativeHistoryCombo = new KHistoryComboBox();
    m_historyCombo = new Plasma::BrowserHistoryComboBox(this);
    m_historyCombo->setNativeWidget(m_nativeHistoryCombo);
    m_historyCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_historyCombo->setZValue(999);

    m_nativeHistoryCombo->setDuplicatesEnabled(false);
    m_pixmapProvider = new KUrlPixmapProvider;
    m_nativeHistoryCombo->setPixmapProvider(m_pixmapProvider);

    m_toolbarLayout->addItem(m_historyCombo);
    m_go = addTool("go-jump-locationbar", m_toolbarLayout);
    m_goAction = m_go->action();
    m_reloadAction = new QAction(KIcon("view-refresh"), QString(), this);

    m_layout->addItem(m_toolbarLayout);

    m_browser = new Plasma::WebView(this);
    m_browser->setPage(new WebBrowserPage(this));
    m_browser->setPreferredSize(400, 400);
    m_browser->setMinimumSize(130, 130);
    m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_layout->addItem(m_browser);

    //bookmarks
    m_bookmarkManager = KBookmarkManager::userBookmarksManager();
    connect(m_bookmarkManager, SIGNAL(changed(QString,QString)), this, SLOT(bookmarksModelInit()));
    bookmarksModelInit();

    m_bookmarksView = new Plasma::TreeView(this);
    m_bookmarksView->setZValue(1);
    m_bookmarksView->nativeWidget()->setAttribute(Qt::WA_NoSystemBackground, false);
    m_bookmarksView->nativeWidget()->verticalScrollBar()->setStyle(QApplication::style());
    m_bookmarksView->nativeWidget()->horizontalScrollBar()->setStyle(QApplication::style());
    m_bookmarksView->setModel(m_bookmarkModel);
    m_bookmarksView->nativeWidget()->setHeaderHidden(true);
    m_bookmarksView->hide();

    m_bookmarksDelegate = new BookmarksDelegate(this);
    m_bookmarksView->nativeWidget()->setItemDelegate(m_bookmarksDelegate);

    connect(m_bookmarksDelegate, SIGNAL(destroyBookmark(QModelIndex)), this, SLOT(removeBookmark(QModelIndex)));

    m_layout->addItem(m_statusbarLayout);

    m_addBookmark = addTool("bookmark-new", m_statusbarLayout);
    m_addBookmarkAction = m_addBookmark->action();
    m_removeBookmarkAction = new QAction(KIcon("list-remove"), QString(), this);
    m_organizeBookmarks = addTool("bookmarks-organize", m_statusbarLayout);

    m_bookmarksViewAnimation = Plasma::Animator::create(Plasma::Animator::FadeAnimation, this);
    m_bookmarksViewAnimation->setTargetWidget(m_bookmarksView);
    connect(m_bookmarksViewAnimation, SIGNAL(finished()), this, SLOT(bookmarksAnimationFinished()));

    m_stop = addTool("process-stop", m_statusbarLayout);

    QGraphicsWidget *spacer = new QGraphicsWidget(this);
    spacer->setMaximumWidth(INT_MAX);
    spacer->setMaximumHeight(0);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_statusbarLayout->addItem(spacer);
    
    m_zoom = new Plasma::Slider(this);
    m_zoom->setMaximum(100);
    m_zoom->setMinimum(0);
    m_zoom->setValue(50);
    m_zoom->setOrientation(Qt::Horizontal);
    m_zoom->hide();
    m_zoom->setMaximumWidth(200);
    m_statusbarLayout->addItem(m_zoom);

    connect(m_zoom, SIGNAL(valueChanged(int)), this, SLOT(zoom(int)));
    m_browser->setUrl(m_url);
    m_browser->update();

    connect(m_back->action(), SIGNAL(triggered()), this, SLOT(back()));
    connect(m_forward->action(), SIGNAL(triggered()), this, SLOT(forward()));
    connect(m_reloadAction, SIGNAL(triggered()), this, SLOT(reload()));
    connect(m_goAction, SIGNAL(triggered()), this, SLOT(returnPressed()));
    connect(m_stop->action(), SIGNAL(triggered()), m_browser->page()->action(QWebPage::Stop), SLOT(trigger()));

    connect(m_historyCombo->nativeWidget(), SIGNAL(returnPressed()), this, SLOT(returnPressed()));
    connect(m_historyCombo->nativeWidget(), SIGNAL(activated(int)), this, SLOT(returnPressed()));
    connect(m_historyCombo, SIGNAL(activated(QString)), this, SLOT(comboTextChanged(QString)));
    connect(m_browser->page()->mainFrame(), SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
    connect(m_browser, SIGNAL(loadProgress(int)), this, SLOT(loadProgress(int)));
    
    connect(m_addBookmarkAction, SIGNAL(triggered()), this, SLOT(addBookmark()));
    connect(m_removeBookmarkAction, SIGNAL(triggered()), this, SLOT(removeBookmark()));
    connect(m_organizeBookmarks->action(), SIGNAL(triggered()), this, SLOT(bookmarksToggle()));
    connect(m_bookmarksView->nativeWidget(), SIGNAL(clicked(QModelIndex)), this, SLOT(bookmarkClicked(QModelIndex)));

    //Autocompletion stuff
    m_completion = new KCompletion();
    m_nativeHistoryCombo->setCompletionObject(m_completion);

    m_graphicsWidget = new QGraphicsWidget(this);
    m_graphicsWidget->setLayout(m_layout);

    m_back->setEnabled(m_browser->page()->history()->canGoBack());
    m_forward->setEnabled(m_browser->page()->history()->canGoForward());

    configChanged();

    connect(this, SIGNAL(messageButtonPressed(MessageButton)), this, SLOT(removeBookmarkMessageButtonPressed(MessageButton)));
    
    return m_graphicsWidget;
}

WebBrowser::~WebBrowser()
{
    KConfigGroup cg = config();
    saveState(cg);
    delete m_completion;
    delete m_bookmarkModel;
}

Plasma::IconWidget *WebBrowser::addTool(const QString &iconString, QGraphicsLinearLayout *layout)
{
    Plasma::IconWidget *icon = new Plasma::IconWidget(this);
    QAction *action = new QAction(KIcon(iconString), QString(), this);
    icon->setAction(action);
    icon->setPreferredSize(icon->sizeFromIconSize(IconSize(KIconLoader::Toolbar)));
    layout->addItem(icon);

    return icon;
}

void WebBrowser::bookmarksModelInit()
{
    if (m_bookmarkModel) {
        m_bookmarkModel->clear();
    } else {
        m_bookmarkModel = new QStandardItemModel;
    }

    fillGroup(0, m_bookmarkManager->root());
}

void WebBrowser::configChanged()
{
    KConfigGroup cg = config();
    
    m_browser->setDragToScroll(cg.readEntry("DragToScroll", false));
    
    if (!m_url.isValid()) {
        m_url = KUrl(cg.readEntry("Url", "http://www.kde.org"));
        m_verticalScrollValue = cg.readEntry("VerticalScrollValue", 0);
        m_horizontalScrollValue = cg.readEntry("HorizontalScrollValue", 0);
        int value = cg.readEntry("Zoom", 50);
        m_zoom->setValue(value);
        qreal zoomFactor = qMax((qreal)0.2, ((qreal)value/(qreal)50));
        if ((zoomFactor > 0.95) && (zoomFactor < 1.05)){
            zoomFactor = 1;
        }
        m_browser->setZoomFactor(zoomFactor);
        m_browser->setUrl(m_url);
    }
    
    m_autoRefresh = cg.readEntry("autoRefresh", false);
    m_autoRefreshInterval = qMax(2, cg.readEntry("autoRefreshInterval", 5));
    
    if (m_autoRefresh) {
        m_autoRefreshTimer = new QTimer(this);
        m_autoRefreshTimer->start(m_autoRefreshInterval*60*1000);
        connect(m_autoRefreshTimer, SIGNAL(timeout()), this, SLOT(reload()));
    }
        
    QStringList list = cg.readEntry("History list", QStringList());
    m_nativeHistoryCombo->setHistoryItems(list);
}

void WebBrowser::fillGroup(BookmarkItem *parentItem, const KBookmarkGroup &group)
{
    KBookmark it = group.first();

    while (!it.isNull()) {
        BookmarkItem *bookmarkItem = new BookmarkItem(it);
        bookmarkItem->setEditable(false);

        if (it.isGroup()) {
            KBookmarkGroup grp = it.toGroup();
            fillGroup( bookmarkItem, grp );

        }

        if (parentItem) {
            parentItem->appendRow(bookmarkItem);
        } else {
            m_bookmarkModel->appendRow(bookmarkItem);
        }

        it = m_bookmarkManager->root().next(it);
    }
}

void WebBrowser::saveState(KConfigGroup &cg) const
{
    cg.writeEntry("Url", m_url.prettyUrl());

    if (m_historyCombo) {
        const QStringList list = m_nativeHistoryCombo->historyItems();
        cg.writeEntry("History list", list);
    }

    if (m_browser) {
        cg.writeEntry("VerticalScrollValue", m_browser->page()->mainFrame()->scrollBarValue(Qt::Vertical));
        cg.writeEntry("HorizontalScrollValue", m_browser->page()->mainFrame()->scrollBarValue(Qt::Horizontal));
    }
}

void WebBrowser::back()
{
    m_browser->page()->history()->back();
}

void WebBrowser::forward()
{
    m_browser->page()->history()->forward();
}

void WebBrowser::reload()
{
    m_browser->setUrl(m_url);
}

void WebBrowser::returnPressed()
{
    KUrl url(m_nativeHistoryCombo->currentText());


    KUriFilter::self()->filterUri( url );

    m_verticalScrollValue = 0;
    m_horizontalScrollValue = 0;
    m_browser->setUrl(url);
}


void WebBrowser::urlChanged(const QUrl &url)
{
    //ask for a favicon
    Plasma::DataEngine *engine = dataEngine( "favicons" );
    if (engine) {
        //engine->disconnectSource( url.toString(), this );
        engine->connectSource( url.toString(), this );

        engine->query( url.toString() );
    }

    m_url = KUrl(url);

    if (m_bookmarkModel->match(m_bookmarkModel->index(0,0), BookmarkItem::UrlRole, m_url.prettyUrl()).isEmpty()) {
        m_addBookmark->setAction(m_addBookmarkAction);
    } else {
        m_addBookmark->setAction(m_removeBookmarkAction);
    }

    m_nativeHistoryCombo->addToHistory(m_url.prettyUrl());
    m_nativeHistoryCombo->setCurrentIndex(0);

    m_go->setAction(m_reloadAction);

    KConfigGroup cg = config();
    saveState(cg);

    m_back->setEnabled(m_browser->page()->history()->canGoBack());
    m_forward->setEnabled(m_browser->page()->history()->canGoForward());
    setAssociatedApplicationUrls(KUrl(url));
}

void WebBrowser::comboTextChanged(const QString &string)
{
    Q_UNUSED(string)

    m_go->setAction(m_goAction);
}

void WebBrowser::dataUpdated( const QString &source, const Plasma::DataEngine::Data &data )
{
    //TODO: find a way to update bookmarks and history combobox here, at the moment the data engine
    // is only used to save the icon files
    if (source == m_nativeHistoryCombo->currentText()) {
        QPixmap favicon(QPixmap::fromImage(data["Icon"].value<QImage>()));
        if (!favicon.isNull()) {
            m_nativeHistoryCombo->setItemIcon(
                                    m_nativeHistoryCombo->currentIndex(), QIcon(favicon));
            setPopupIcon(QIcon(favicon));
        }
    }
}

void WebBrowser::addBookmark()
{
    KBookmark bookmark = m_bookmarkManager->root().addBookmark(m_browser->page()->mainFrame()->title(), m_url);
    m_bookmarkManager->save();

    BookmarkItem *bookmarkItem = new BookmarkItem(bookmark);
    m_bookmarkModel->appendRow(bookmarkItem);

    m_addBookmark->setAction(m_removeBookmarkAction);
}

void WebBrowser::removeBookmark(const QModelIndex &index)
{
    BookmarkItem *item = dynamic_cast<BookmarkItem *>(m_bookmarkModel->itemFromIndex(index));

    if (item) {
        KBookmark bookmark = item->bookmark();

        const QString text(i18nc("@info", "Do you really want to remove the bookmark to %1?", bookmark.url().host()));
        showMessage(KIcon("dialog-warning"), text, Plasma::ButtonYes | Plasma::ButtonNo);
        return;
    }

    if (item && item->parent()) {
        item->parent()->removeRow(index.row());
    } else {
        m_bookmarkModel->removeRow(index.row());
    }

}

void WebBrowser::removeBookmarkMessageButtonPressed(const Plasma::MessageButton button)
{  
    if (button == Plasma::ButtonNo){
        return;
    }
    
    const QModelIndexList list = m_bookmarkModel->match(m_bookmarkModel->index(0,0), BookmarkItem::UrlRole, m_url.prettyUrl());

    if (!list.isEmpty()) {
        const QModelIndex &index = list.first();
        BookmarkItem *item = dynamic_cast<BookmarkItem *>(m_bookmarkModel->itemFromIndex(index));
        
        if (item) {
            KBookmark bookmark = item->bookmark();
            
            bookmark.parentGroup().deleteBookmark(bookmark);
            m_bookmarkManager->save();
        }
        
        if (item && item->parent()) {
            item->parent()->removeRow(index.row());
        } else {
            m_bookmarkModel->removeRow(index.row());
        }
    }
    
    m_addBookmark->setAction(m_addBookmarkAction);
}
void WebBrowser::removeBookmark()
{
    const QModelIndexList list = m_bookmarkModel->match(m_bookmarkModel->index(0,0), BookmarkItem::UrlRole, m_url.prettyUrl());

    if (!list.isEmpty()) {
        removeBookmark(list.first());
    }
}

void WebBrowser::bookmarksToggle()
{
    if (m_bookmarksView->isVisible()) {
        m_bookmarksViewAnimation->setProperty("startOpacity", 1);
        m_bookmarksViewAnimation->setProperty("targetOpacity", 0);
        m_bookmarksViewAnimation->start();
    } else {
        m_bookmarksView->show();
        m_bookmarksView->setOpacity(0);
        updateOverlaysGeometry();
        m_bookmarksViewAnimation->setProperty("startOpacity", 0);
        m_bookmarksViewAnimation->setProperty("targetOpacity", 1);
        m_bookmarksViewAnimation->start();
    }
}

void WebBrowser::bookmarksAnimationFinished()
{
    if (qFuzzyCompare(m_bookmarksView->opacity() + 1, 1)){
        m_bookmarksView->hide();
    }
}

void WebBrowser::bookmarkClicked(const QModelIndex &index)
{
    QStandardItem *item = m_bookmarkModel->itemFromIndex(index);

    if (item) {
        KUrl url(item->data(BookmarkItem::UrlRole).value<QString>());

        if (url.isValid()) {
            m_browser->setUrl(url);
            bookmarksToggle();
        }
    }
}

void WebBrowser::zoom(int value)
{
    config().writeEntry("Zoom", value);
    m_browser->setZoomFactor((qreal)0.2 + ((qreal)value/(qreal)50));
}

void WebBrowser::loadProgress(int progress)
{
    m_historyCombo->setProgressValue(progress);

    if (progress == 100) {
        m_historyCombo->setDisplayProgress(false);
        m_stop->hide();
        m_stop->setMaximumWidth(0);
        m_zoom->show();
        m_statusbarLayout->invalidate();

        m_browser->page()->mainFrame()->setScrollBarValue(Qt::Vertical, m_verticalScrollValue);
        m_browser->page()->mainFrame()->setScrollBarValue(Qt::Horizontal, m_horizontalScrollValue);

    } else {
        m_historyCombo->setDisplayProgress(true);
        m_stop->show();
        m_stop->setMaximumWidth(INT_MAX);
        m_zoom->hide();
        m_statusbarLayout->invalidate();
    }
}

void WebBrowser::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    parent->addPage(widget, i18n("General"), icon());
    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));

    ui.autoRefresh->setChecked(m_autoRefresh);
    ui.autoRefreshInterval->setValue(m_autoRefreshInterval);
    ui.autoRefreshInterval->setSuffix(ki18np(" minute", " minutes"));
    ui.dragToScroll->setChecked(m_browser->dragToScroll());
    connect(ui.autoRefresh, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.dragToScroll, SIGNAL(toggled(bool)), parent, SLOT(settingsModified()));
    connect(ui.autoRefreshInterval, SIGNAL(valueChanged(int)), parent, SLOT(settingsModified()));
}

void WebBrowser::configAccepted()
{
    KConfigGroup cg = config();

    m_autoRefresh = ui.autoRefresh->isChecked();
    m_autoRefreshInterval = ui.autoRefreshInterval->value();

    cg.writeEntry("autoRefresh", m_autoRefresh);
    cg.writeEntry("autoRefreshInterval", m_autoRefreshInterval);
    cg.writeEntry("DragToScroll", ui.dragToScroll->isChecked());
    m_browser->setDragToScroll(ui.dragToScroll->isChecked());

    if (m_autoRefresh) {
        if (!m_autoRefreshTimer) {
            m_autoRefreshTimer = new QTimer(this);
            connect(m_autoRefreshTimer, SIGNAL(timeout()), this, SLOT(reload()));
        }

        m_autoRefreshTimer->start(m_autoRefreshInterval*60*1000);
    } else {
        delete m_autoRefreshTimer;
        m_autoRefreshTimer = 0;
    }

    emit configNeedsSaving();
}

void WebBrowser::constraintsEvent(Plasma::Constraints constraints)
{
    if (constraints & Plasma::SizeConstraint){
        updateOverlaysGeometry();
    }
}

void WebBrowser::updateOverlaysGeometry()
{
    QRect overlayGeometry(m_browser->pos().x() + contentsRect().x(),
                          m_browser->pos().y() + contentsRect().y(),
                          m_browser->geometry().width(),
                          m_browser->geometry().height());

    if (m_bookmarksView->isVisible()) {
      m_bookmarksView->setGeometry(overlayGeometry);
    }
      
    if (m_webOverlay){
      m_webOverlay->setGeometry(overlayGeometry);
    }
}

void WebBrowser::paintInterface(QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    Q_UNUSED(option)
    Q_UNUSED(contentsRect)

    if (!isIconified()){
        p->save();
        p->setBrush(QApplication::palette().window());
        p->setRenderHint(QPainter::Antialiasing);
        p->setPen(Qt::NoPen);
        p->drawRoundedRect(m_browser->pos().x() + contentsRect.x() - 2,
                        m_browser->pos().y() + contentsRect.y() - 2,
                        m_browser->geometry().width() + 4,
                        m_browser->geometry().height() + 4,
                        2, 2);
        p->restore();
    }
}

void WebBrowser::closeWebViewOverlay()
{
    if (m_webOverlay){
      m_webOverlay->deleteLater();
      m_webOverlay = 0;
    }
}

QWebPage *WebBrowser::createWindow(QWebPage::WebWindowType type)
{
    Q_UNUSED(type)
  
    if (!m_webOverlay){
        m_webOverlay = new WebViewOverlay(this);
        updateOverlaysGeometry();
        m_webOverlay->setZValue(999);
        connect(m_webOverlay, SIGNAL(closeRequested()), this, SLOT(closeWebViewOverlay()));
    }

    return m_webOverlay->page();
}

//
// Wallet managment
//

void WebBrowser::saveFormDataRequested(const QString &uid, const QUrl &url)
{
    BrowserMessageBox *messageBox = new BrowserMessageBox(this, i18n("Do you want to store this password for %1?", url.host()));
    messageBox->okButton()->setText(i18n("Store"));
    messageBox->okButton()->setIcon(KIcon("document-save"));
    messageBox->cancelButton()->setText(i18n("Do not store this time"));
    messageBox->cancelButton()->setIcon(KIcon("dialog-cancel"));
    m_layout->insertItem(1, messageBox);
    walletRequests.insert(messageBox, uid);
    connect(messageBox, SIGNAL(okClicked()), this, SLOT(acceptWalletRequest()));
    connect(messageBox, SIGNAL(cancelClicked()), this, SLOT(rejectWalletRequest()));
}

void WebBrowser::acceptWalletRequest()
{
    static_cast<KWebPage *>(m_browser->page())->wallet()->acceptSaveFormDataRequest(
                            walletRequests[static_cast<BrowserMessageBox *>(QObject::sender())]);
    QObject::sender()->deleteLater();
}

void WebBrowser::rejectWalletRequest()
{
    static_cast<KWebPage *>(m_browser->page())->wallet()->rejectSaveFormDataRequest(
                           walletRequests[static_cast<BrowserMessageBox *>(QObject::sender())]);
    QObject::sender()->deleteLater();
}

//
// End of wallet managment
//

K_EXPORT_PLASMA_APPLET(webbrowser, WebBrowser)

#include "webbrowser.moc"
