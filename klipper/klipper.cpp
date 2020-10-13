/* This file is part of the KDE project

   Copyright (C) by Andrew Stanley-Jones <asj@cban.com>
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>
   Copyright (C) 2008 by Dmitry Suzdalev <dimsuz@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "klipper.h"

#include <zlib.h>

#include "klipper_debug.h"
#include <QDir>
#include <QDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QDBusConnection>
#include <QSaveFile>
#include <QtConcurrent>

#include <KGlobalAccel>
#include <KMessageBox>
#include <KNotification>
#include <KActionCollection>
#include <KToggleAction>
#include <KTextEdit>
#include <KWindowSystem>

#include "configdialog.h"
#include "klippersettings.h"
#include "urlgrabber.h"
#include "history.h"
#include "historyitem.h"
#include "historymodel.h"
#include "historystringitem.h"
#include "klipperpopup.h"

#include "systemclipboard.h"

#include <prison/Prison>

#include <config-X11.h>
#if HAVE_X11
#include <QX11Info>
#include <xcb/xcb.h>
#endif

namespace {
    /**
     * Use this when manipulating the clipboard
     * from within clipboard-related signals.
     *
     * This avoids issues such as mouse-selections that immediately
     * disappear.
     * pattern: Resource Acquisition is Initialisation (RAII)
     *
     * (This is not threadsafe, so don't try to use such in threaded
     * applications).
     */
    struct Ignore {
        Ignore(int& locklevel) : locklevelref(locklevel)  {
            locklevelref++;
        }
        ~Ignore() {
            locklevelref--;
        }
    private:
        int& locklevelref;
    };
}

// config == KGlobal::config for process, otherwise applet
Klipper::Klipper(QObject* parent, const KSharedConfigPtr& config, KlipperMode mode)
    : QObject( parent )
    , m_overflowCounter( 0 )
    , m_locklevel( 0 )
    , m_config( config )
    , m_pendingContentsCheck( false )
    , m_mode(mode)
{
    if (m_mode == KlipperMode::Standalone) {
        setenv("KSNI_NO_DBUSMENU", "1", 1);
    }
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.klipper"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/klipper"), this, QDBusConnection::ExportScriptableSlots);

    updateTimestamp(); // read initial X user time
    m_clip = SystemClipboard::instance();

    connect( m_clip, &SystemClipboard::changed, this, &Klipper::newClipData );

    connect( &m_overflowClearTimer, &QTimer::timeout, this, &Klipper::slotClearOverflow);

    m_pendingCheckTimer.setSingleShot( true );
    connect( &m_pendingCheckTimer, &QTimer::timeout, this, &Klipper::slotCheckPending);


    m_history = new History( this );
    m_popup = new KlipperPopup(m_history);
    m_popup->setShowHelp(m_mode == KlipperMode::Standalone);
    connect(m_history, &History::changed, this, &Klipper::slotHistoryChanged);
    connect(m_history, &History::changed, m_popup, &KlipperPopup::slotHistoryChanged);
    connect(m_history, &History::topIsUserSelectedSet, m_popup, &KlipperPopup::slotTopIsUserSelectedSet);

    // we need that collection, otherwise KToggleAction is not happy :}
    m_collection = new KActionCollection( this );

    m_toggleURLGrabAction = new KToggleAction( this );
    m_collection->addAction( QStringLiteral("clipboard_action"), m_toggleURLGrabAction );
    m_toggleURLGrabAction->setText(i18n("Enable Clipboard Actions"));
    KGlobalAccel::setGlobalShortcut(m_toggleURLGrabAction, QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_X));
    connect( m_toggleURLGrabAction, &QAction::toggled,
             this, &Klipper::setURLGrabberEnabled);

    /*
     * Create URL grabber
     */
    m_myURLGrabber = new URLGrabber(m_history);
    connect( m_myURLGrabber, &URLGrabber::sigPopup,
            this, &Klipper::showPopupMenu );
    connect( m_myURLGrabber, &URLGrabber::sigDisablePopup,
            this, &Klipper::disableURLGrabber );

    /*
     * Load configuration settings
     */
    loadSettings();

    // load previous history if configured
    if (m_bKeepContents) {
        loadHistory();
    }

    m_clearHistoryAction = m_collection->addAction( QStringLiteral("clear-history") );
    m_clearHistoryAction->setIcon( QIcon::fromTheme(QStringLiteral("edit-clear-history")) );
    m_clearHistoryAction->setText( i18n("C&lear Clipboard History") );
    KGlobalAccel::setGlobalShortcut(m_clearHistoryAction, QKeySequence());
    connect(m_clearHistoryAction, &QAction::triggered, this, &Klipper::slotAskClearHistory);

    QString CONFIGURE=QStringLiteral("configure");
    m_configureAction = m_collection->addAction( CONFIGURE );
    m_configureAction->setIcon( QIcon::fromTheme(CONFIGURE) );
    m_configureAction->setText( i18n("&Configure Klipper...") );
    connect(m_configureAction, &QAction::triggered, this, &Klipper::slotConfigure);

    m_quitAction = m_collection->addAction( QStringLiteral("quit") );
    m_quitAction->setIcon( QIcon::fromTheme(QStringLiteral("application-exit")) );
    m_quitAction->setText( i18nc("@item:inmenu Quit Klipper", "&Quit") );
    connect(m_quitAction, &QAction::triggered, this, &Klipper::slotQuit);

    m_repeatAction = m_collection->addAction(QStringLiteral("repeat_action"));
    m_repeatAction->setText(i18n("Manually Invoke Action on Current Clipboard"));
    KGlobalAccel::setGlobalShortcut(m_repeatAction, QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_R));
    connect(m_repeatAction, &QAction::triggered, this, &Klipper::slotRepeatAction);

    // add an edit-possibility
    m_editAction = m_collection->addAction(QStringLiteral("edit_clipboard"));
    m_editAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_editAction->setText(i18n("&Edit Contents..."));
    KGlobalAccel::setGlobalShortcut(m_editAction, QKeySequence());
    connect(m_editAction, &QAction::triggered, this,
        [this]() {
            editData(m_history->first());
        }
    );

    // add barcode for mobile phones
    m_showBarcodeAction = m_collection->addAction(QStringLiteral("show-barcode"));
    m_showBarcodeAction->setText(i18n("&Show Barcode..."));
    KGlobalAccel::setGlobalShortcut(m_showBarcodeAction, QKeySequence());
    connect(m_showBarcodeAction, &QAction::triggered, this,
        [this]() {
            showBarcode(m_history->first());
        }
    );

    // Cycle through history
    m_cycleNextAction = m_collection->addAction(QStringLiteral("cycleNextAction"));
    m_cycleNextAction->setText(i18n("Next History Item"));
    KGlobalAccel::setGlobalShortcut(m_cycleNextAction, QKeySequence());
    connect(m_cycleNextAction, &QAction::triggered, this, &Klipper::slotCycleNext);
    m_cyclePrevAction = m_collection->addAction(QStringLiteral("cyclePrevAction"));
    m_cyclePrevAction->setText(i18n("Previous History Item"));
    KGlobalAccel::setGlobalShortcut(m_cyclePrevAction, QKeySequence());
    connect(m_cyclePrevAction, &QAction::triggered, this, &Klipper::slotCyclePrev);

    // Action to show Klipper popup on mouse position
    m_showOnMousePos = m_collection->addAction(QStringLiteral("show-on-mouse-pos"));
    m_showOnMousePos->setText(i18n("Open Klipper at Mouse Position"));
    KGlobalAccel::setGlobalShortcut(m_showOnMousePos, QKeySequence());
    connect(m_showOnMousePos, &QAction::triggered, this, &Klipper::slotPopupMenu);

    connect ( history(), &History::topChanged, this, &Klipper::slotHistoryTopChanged );
    connect( m_popup, &QMenu::aboutToShow, this, &Klipper::slotStartShowTimer );

    if (m_mode == KlipperMode::Standalone) {
        m_popup->plugAction( m_toggleURLGrabAction );
        m_popup->plugAction( m_clearHistoryAction );
        m_popup->plugAction( m_configureAction );
        m_popup->plugAction( m_repeatAction );
        m_popup->plugAction( m_editAction );
        m_popup->plugAction( m_showBarcodeAction );
        m_popup->plugAction( m_quitAction );
    }

    // session manager interaction
    if (m_mode == KlipperMode::Standalone) {
        connect(qApp, &QGuiApplication::commitDataRequest, this, &Klipper::saveSession);
    }

    connect(this, &Klipper::passivePopup, this,
        [this] (const QString &caption, const QString &text) {
            if (m_notification) {
                m_notification->setTitle(caption);
                m_notification->setText(text);
            } else {
                m_notification = KNotification::event(KNotification::Notification, caption, text, QStringLiteral("klipper"));
                // When Klipper is run as part of plasma, we still need to pretend to be it for notification settings to work
                m_notification->setHint(QStringLiteral("desktop-entry"), QStringLiteral("org.kde.klipper"));
            }
        }
    );
}

Klipper::~Klipper()
{
    delete m_myURLGrabber;
}

// DBUS
QString Klipper::getClipboardContents()
{
    return getClipboardHistoryItem(0);
}

void Klipper::showKlipperPopupMenu()
{
    slotPopupMenu();
}

void Klipper::showKlipperManuallyInvokeActionMenu()
{
    slotRepeatAction();
}


// DBUS - don't call from Klipper itself
void Klipper::setClipboardContents(const QString &s)
{
    if (s.isEmpty())
        return;
    Ignore lock( m_locklevel );
    updateTimestamp();
    HistoryItemPtr item(HistoryItemPtr(new HistoryStringItem(s)));
    setClipboard( *item, Clipboard | Selection);
    history()->insert( item );
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardContents()
{
    updateTimestamp();
    slotClearClipboard();
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardHistory()
{
    updateTimestamp();
    history()->slotClear();
    saveSession();
}

// DBUS - don't call from Klipper itself
void Klipper::saveClipboardHistory()
{
    if ( m_bKeepContents ) { // save the clipboard eventually
        saveHistory();
    }
}

void Klipper::slotStartShowTimer()
{
    m_showTimer.start();
}

void Klipper::loadSettings()
{
    // Security bug 142882: If user has save clipboard turned off, old data should be deleted from disk
    static bool firstrun = true;
    if (!firstrun && m_bKeepContents && !KlipperSettings::keepClipboardContents()) {
        saveHistory(true);
    }
    firstrun=false;

    m_bKeepContents = KlipperSettings::keepClipboardContents();
    m_bReplayActionInHistory = KlipperSettings::replayActionInHistory();
    m_bNoNullClipboard = KlipperSettings::preventEmptyClipboard();
    // 0 is the id of "Ignore selection" radiobutton
    m_bIgnoreSelection = KlipperSettings::ignoreSelection();
    m_bIgnoreImages = KlipperSettings::ignoreImages();
    m_bSynchronize = KlipperSettings::syncClipboards();
    // NOTE: not used atm - kregexpeditor is not ported to kde4
    m_bUseGUIRegExpEditor = KlipperSettings::useGUIRegExpEditor();
    m_bSelectionTextOnly = KlipperSettings::selectionTextOnly();

    m_bURLGrabber = KlipperSettings::uRLGrabberEnabled();
    // this will cause it to loadSettings too
    setURLGrabberEnabled(m_bURLGrabber);
    history()->setMaxSize( KlipperSettings::maxClipItems() );
    history()->model()->setDisplayImages(!m_bIgnoreImages);

    // Convert 4.3 settings
    if (KlipperSettings::synchronize() != 3) {
      // 2 was the id of "Ignore selection" radiobutton
      m_bIgnoreSelection = KlipperSettings::synchronize() == 2;
      // 0 was the id of "Synchronize contents" radiobutton
      m_bSynchronize = KlipperSettings::synchronize() == 0;
      KConfigSkeletonItem* item = KlipperSettings::self()->findItem(QStringLiteral("SyncClipboards"));
      item->setProperty(m_bSynchronize);
      item = KlipperSettings::self()->findItem(QStringLiteral("IgnoreSelection"));
      item->setProperty(m_bIgnoreSelection);
      item =  KlipperSettings::self()->findItem(QStringLiteral("Synchronize")); // Mark property as converted.
      item->setProperty(3);
      KlipperSettings::self()->save();
      KlipperSettings::self()->load();

    }

    if (m_bKeepContents && !m_saveFileTimer) {
        m_saveFileTimer = new QTimer(this);
        m_saveFileTimer->setSingleShot(true);
        m_saveFileTimer->setInterval(5000);
        connect(m_saveFileTimer, &QTimer::timeout, this,
            [this] {
                QtConcurrent::run(this, &Klipper::saveHistory, false);
            }
        );
        connect(m_history, &History::changed, m_saveFileTimer, static_cast<void (QTimer::*)()>(&QTimer::start));
    } else {
        delete m_saveFileTimer;
        m_saveFileTimer = nullptr;
    }
}

void Klipper::saveSettings() const
{
    m_myURLGrabber->saveSettings();
    KlipperSettings::self()->setVersion(QStringLiteral(KLIPPER_VERSION_STRING));
    KlipperSettings::self()->save();

    // other settings should be saved automatically by KConfigDialog
}

void Klipper::showPopupMenu( QMenu* menu )
{
    Q_ASSERT( menu != nullptr );

    menu->popup(QCursor::pos());
}

bool Klipper::loadHistory() {
    static const char failed_load_warning[] =
        "Failed to load history resource. Clipboard history cannot be read.";
    // don't use "appdata", klipper is also a kicker applet
    QFile history_file(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                              QStringLiteral("klipper/history2.lst")));
    if ( !history_file.exists() ) {
        qCWarning(KLIPPER_LOG) << failed_load_warning << ": " << "History file does not exist" ;
        return false;
    }
    if ( !history_file.open( QIODevice::ReadOnly ) ) {
        qCWarning(KLIPPER_LOG) << failed_load_warning << ": " << history_file.errorString() ;
        return false;
    }
    QDataStream file_stream( &history_file );
    if( file_stream.atEnd()) {
        qCWarning(KLIPPER_LOG) << failed_load_warning << ": " << "Error in reading data" ;
        return false;
    }
    QByteArray data;
    quint32 crc;
    file_stream >> crc >> data;
    if( crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() ) != crc ) {
        qCWarning(KLIPPER_LOG) << failed_load_warning << ": " << "CRC checksum does not match" ;
        return false;
    }
    QDataStream history_stream( &data, QIODevice::ReadOnly );

    char* version;
    history_stream >> version;
    delete[] version;

    // The list needs to be reversed, as it is saved
    // youngest-first to keep the most important clipboard
    // items at the top, but the history is created oldest
    // first.
    QVector<HistoryItemPtr> reverseList;
    for ( HistoryItemPtr item = HistoryItem::create( history_stream );
          !item.isNull();
          item = HistoryItem::create( history_stream ) )
    {
        reverseList.prepend( item );
    }

    history()->slotClear();

    for ( auto it = reverseList.constBegin();
          it != reverseList.constEnd();
          ++it )
    {
        history()->forceInsert(*it);
    }

    if ( !history()->empty() ) {
        setClipboard( *history()->first(), Clipboard | Selection );
    }

    return true;
}

void Klipper::saveHistory(bool empty) {
    QMutexLocker lock(m_history->model()->mutex());
    static const char failed_save_warning[] =
        "Failed to save history. Clipboard history cannot be saved.";
    // don't use "appdata", klipper is also a kicker applet
    QString history_file_name(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("klipper/history2.lst")));
    if ( history_file_name.isNull() || history_file_name.isEmpty() ) {
        // try creating the file
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
        if (!dir.mkpath(QStringLiteral("klipper"))) {
            qCWarning(KLIPPER_LOG) << failed_save_warning ;
            return;
        }
        history_file_name = dir.absoluteFilePath(QStringLiteral("klipper/history2.lst"));
    }
    if ( history_file_name.isNull() || history_file_name.isEmpty() ) {
        qCWarning(KLIPPER_LOG) << failed_save_warning ;
        return;
    }
    QSaveFile history_file( history_file_name );
    if (!history_file.open(QIODevice::WriteOnly)) {
        qCWarning(KLIPPER_LOG) << failed_save_warning ;
        return;
    }
    QByteArray data;
    QDataStream history_stream( &data, QIODevice::WriteOnly );
    history_stream << KLIPPER_VERSION_STRING; // const char*

    if (!empty) {
        HistoryItemConstPtr item = history()->first();
        if (item) {
            do {
                history_stream << item.data();
                item = HistoryItemConstPtr(history()->find(item->next_uuid()));
            } while (item != history()->first());
        }
    }

    quint32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    QDataStream ds ( &history_file );
    ds << crc << data;
    if (!history_file.commit()) {
        qCWarning(KLIPPER_LOG) << failed_save_warning ;
    }
}

// save session on shutdown. Don't simply use the c'tor, as that may not be called.
void Klipper::saveSession()
{
    if ( m_bKeepContents ) { // save the clipboard eventually
        saveHistory();
    }
    saveSettings();
}

void Klipper::disableURLGrabber()
{
    QMessageBox *message = new QMessageBox(QMessageBox::Information, QString(),
        i18n("You can enable URL actions later by left-clicking on the "
             "Klipper icon and selecting 'Enable Clipboard Actions'"));
    message->setAttribute(Qt::WA_DeleteOnClose);
    message->setModal(false);
    message->show();

    setURLGrabberEnabled( false );
}

void Klipper::slotConfigure()
{
    if (KConfigDialog::showDialog(QStringLiteral("preferences"))) {
        return;
    }

    ConfigDialog *dlg = new ConfigDialog( nullptr, KlipperSettings::self(), this, m_collection );
    QMetaObject::invokeMethod(dlg, "setHelp", Qt::DirectConnection, Q_ARG(QString, QString::fromLatin1("")), Q_ARG(QString, QString::fromLatin1("klipper")));

    connect(dlg, &KConfigDialog::settingsChanged, this, &Klipper::loadSettings);

    dlg->show();
}

void Klipper::slotQuit()
{
    // If the menu was just opened, likely the user
    // selected quit by accident while attempting to
    // click the Klipper icon.
    if ( m_showTimer.elapsed() < 300 ) {
        return;
    }

    saveSession();
    int autoStart = KMessageBox::questionYesNoCancel(nullptr, i18n("Should Klipper start automatically when you login?"),
                                                     i18n("Automatically Start Klipper?"), KGuiItem(i18n("Start")),
                                                     KGuiItem(i18n("Do Not Start")), KStandardGuiItem::cancel(), QStringLiteral("StartAutomatically"));

    KConfigGroup config( KSharedConfig::openConfig(), "General");
    if ( autoStart == KMessageBox::Yes ) {
        config.writeEntry("AutoStart", true);
    } else if ( autoStart == KMessageBox::No) {
        config.writeEntry("AutoStart", false);
    } else  // cancel chosen don't quit
        return;
    config.sync();

    qApp->quit();

}

void Klipper::slotPopupMenu() {
    m_popup->ensureClean();
    m_popup->slotSetTopActive();
    showPopupMenu( m_popup );
}


void Klipper::slotRepeatAction()
{
    auto top = qSharedPointerCast<const HistoryStringItem>( history()->first() );
    if ( top ) {
        m_myURLGrabber->invokeAction( top );
    }
}

void Klipper::setURLGrabberEnabled( bool enable )
{
    if (enable != m_bURLGrabber) {
      m_bURLGrabber = enable;
      m_lastURLGrabberTextSelection.clear();
      m_lastURLGrabberTextClipboard.clear();
      KlipperSettings::setURLGrabberEnabled(enable);
    }

    m_toggleURLGrabAction->setChecked( enable );

    // make it update its settings
    m_myURLGrabber->loadSettings();
}

void Klipper::slotHistoryTopChanged() {
    if ( m_locklevel ) {
        return;
    }

    auto topitem = history()->first();
    if ( topitem ) {
        setClipboard( *topitem, Clipboard | Selection );
    }
    if ( m_bReplayActionInHistory && m_bURLGrabber ) {
        slotRepeatAction();
    }
}

void Klipper::slotClearClipboard()
{
    Ignore lock( m_locklevel );

    m_clip->clear(QClipboard::Selection);
    m_clip->clear(QClipboard::Clipboard);
}

HistoryItemPtr Klipper::applyClipChanges( const QMimeData* clipData )
{
    if ( m_locklevel ) {
        return HistoryItemPtr();
    }
    Ignore lock( m_locklevel );

    if (!(history()->empty())) {
        if (m_bIgnoreImages && history()->first()->mimeData()->hasImage()) {
            history()->remove(history()->first());
        }
    }

    HistoryItemPtr item = HistoryItem::create( clipData );

    bool saveToHistory = true;
    if (clipData->data(QStringLiteral("x-kde-passwordManagerHint")) == QByteArrayLiteral("secret")) {
        saveToHistory = false;
    }
    if (saveToHistory) {
        history()->insert( item );
    }

    return item;
}

void Klipper::newClipData( QClipboard::Mode mode )
{
    if ( m_locklevel ) {
        return;
    }

    if( mode == QClipboard::Selection && blockFetchingNewData())
        return;

    checkClipData( mode == QClipboard::Selection ? true : false );

}

void Klipper::slotHistoryChanged()
{
    if (history()->empty()) {
        slotClearClipboard();
    }
}

// Protection against too many clipboard data changes. Lyx responds to clipboard data
// requests with setting new clipboard data, so if Lyx takes over clipboard,
// Klipper notices, requests this data, this triggers "new" clipboard contents
// from Lyx, so Klipper notices again, requests this data, ... you get the idea.
const int MAX_CLIPBOARD_CHANGES = 10; // max changes per second

bool Klipper::blockFetchingNewData()
{
#if HAVE_X11
// Hacks for #85198 and #80302.
// #85198 - block fetching new clipboard contents if Shift is pressed and mouse is not,
//   this may mean the user is doing selection using the keyboard, in which case
//   it's possible the app sets new clipboard contents after every change - Klipper's
//   history would list them all.
// #80302 - OOo (v1.1.3 at least) has a bug that if Klipper requests its clipboard contents
//   while the user is doing a selection using the mouse, OOo stops updating the clipboard
//   contents, so in practice it's like the user has selected only the part which was
//   selected when Klipper asked first.
// Use XQueryPointer rather than QApplication::mouseButtons()/keyboardModifiers(), because
//   Klipper needs the very current state.
    if (!KWindowSystem::isPlatformX11()) {
        return false;
    }
    xcb_connection_t *c = QX11Info::connection();
    const xcb_query_pointer_cookie_t cookie = xcb_query_pointer_unchecked(c, QX11Info::appRootWindow());
    QScopedPointer<xcb_query_pointer_reply_t, QScopedPointerPodDeleter> queryPointer(xcb_query_pointer_reply(c, cookie, nullptr));
    if (queryPointer.isNull()) {
        return false;
    }
    if (((queryPointer->mask & (XCB_KEY_BUT_MASK_SHIFT | XCB_KEY_BUT_MASK_BUTTON_1)) == XCB_KEY_BUT_MASK_SHIFT) // BUG: 85198
            || ((queryPointer->mask & XCB_KEY_BUT_MASK_BUTTON_1) == XCB_KEY_BUT_MASK_BUTTON_1)) { // BUG: 80302
        m_pendingContentsCheck = true;
        m_pendingCheckTimer.start( 100 );
        return true;
    }
    m_pendingContentsCheck = false;
    if ( m_overflowCounter == 0 )
        m_overflowClearTimer.start( 1000 );
    if( ++m_overflowCounter > MAX_CLIPBOARD_CHANGES )
        return true;
#endif
    return false;
}

void Klipper::slotCheckPending()
{
    if( !m_pendingContentsCheck )
        return;
    m_pendingContentsCheck = false; // blockFetchingNewData() will be called again
    updateTimestamp();
    newClipData( QClipboard::Selection ); // always selection
}

void Klipper::checkClipData( bool selectionMode )
{
    if ( ignoreClipboardChanges() ) // internal to klipper, ignoring QSpinBox selections
    {
        // keep our old clipboard, thanks
        // This won't quite work, but it's close enough for now.
        // The trouble is that the top selection =! top clipboard
        // but we don't track that yet. We will....
        auto top = history()->first();
        if ( top ) {
            setClipboard( *top, selectionMode ? Selection : Clipboard);
        }
        return;
    }

    qCDebug(KLIPPER_LOG) << "Checking clip data";

    const QMimeData* data = m_clip->mimeData( selectionMode ? QClipboard::Selection : QClipboard::Clipboard );

    bool clipEmpty = false;
    bool changed = true; // ### FIXME (only relevant under polling, might be better to simply remove polling and rely on XFixes)
    if ( !data ) {
        clipEmpty = true;
    } else {
        clipEmpty = data->formats().isEmpty();
        if (clipEmpty) {
            // Might be a timeout. Try again
            clipEmpty = data->formats().isEmpty();
            qCDebug(KLIPPER_LOG) << "was empty. Retried, now " << (clipEmpty?" still empty":" no longer empty");
        }
    }

    if ( changed && clipEmpty && m_bNoNullClipboard ) {
        auto top = history()->first();
        if ( top ) {
            // keep old clipboard after someone set it to null
            qCDebug(KLIPPER_LOG) << "Resetting clipboard (Prevent empty clipboard)";
            setClipboard( *top, selectionMode ? Selection : Clipboard, ClipboardUpdateReason::PreventEmptyClipboard);
        }
        return;
    } else if (clipEmpty) {
        return;
    }

    // this must be below the "bNoNullClipboard" handling code!
    // XXX: I want a better handling of selection/clipboard in general.
    // XXX: Order sensitive code. Must die.
    if ( selectionMode && m_bIgnoreSelection )
        return;

    if( selectionMode && m_bSelectionTextOnly && !data->hasText())
        return;

    if( data->hasUrls() )
        ; // ok
    else if( data->hasText() )
        ; // ok
    else if( data->hasImage() )
    {
        if (m_bIgnoreImages && !data->hasFormat(QStringLiteral("x-kde-force-image-copy")))
            return;
    }
    else // unknown, ignore
        return;

    HistoryItemPtr item = applyClipChanges( data );
    if (changed) {
        qCDebug(KLIPPER_LOG) << "Synchronize?" << m_bSynchronize;
        if ( m_bSynchronize && item ) {
            setClipboard( *item, selectionMode ? Clipboard : Selection );
        }
    }
    QString& lastURLGrabberText = selectionMode
        ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if( m_bURLGrabber && item && data->hasText())
    {
        m_myURLGrabber->checkNewData( qSharedPointerConstCast<const HistoryItem>(item) );

        // Make sure URLGrabber doesn't repeat all the time if klipper reads the same
        // text all the time (e.g. because XFixes is not available and the application
        // has broken TIMESTAMP target). Using most recent history item may not always
        // work.
        if ( item->text() != lastURLGrabberText )
        {
            lastURLGrabberText = item->text();
        }
    } else {
        lastURLGrabberText.clear();
    }
}

void Klipper::setClipboard( const HistoryItem& item, int mode , ClipboardUpdateReason updateReason)
{
    Ignore lock( m_locklevel );

    Q_ASSERT( ( mode & 1 ) == 0 ); // Warn if trying to pass a boolean as a mode.

    if ( mode & Selection ) {
        qCDebug(KLIPPER_LOG) << "Setting selection to <" << item.text() << ">";
        QMimeData *mimeData = item.mimeData();
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            mimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        }
        m_clip->setMimeData( mimeData, QClipboard::Selection );
    }
    if ( mode & Clipboard ) {
        qCDebug(KLIPPER_LOG) << "Setting clipboard to <" << item.text() << ">";
        QMimeData *mimeData = item.mimeData();
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            mimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        }
        m_clip->setMimeData( mimeData, QClipboard::Clipboard );
    }

}

void Klipper::slotClearOverflow()
{
    m_overflowClearTimer.stop();

    if( m_overflowCounter > MAX_CLIPBOARD_CHANGES ) {
        qCDebug(KLIPPER_LOG) << "App owning the clipboard/selection is lame";
        // update to the latest data - this unfortunately may trigger the problem again
        newClipData( QClipboard::Selection ); // Always the selection.
    }
    m_overflowCounter = 0;
}

QStringList Klipper::getClipboardHistoryMenu()
{
    QStringList menu;
    auto item = history()->first();
    if (item) {
        do {
            menu << item->text();
            item = history()->find(item->next_uuid());
        } while (item != history()->first());
    }

    return menu;
}

QString Klipper::getClipboardHistoryItem(int i)
{
    auto item = history()->first();
    if (item) {
        do {
            if (i-- == 0) {
                return item->text();
            }
            item = history()->find(item->next_uuid());
        } while (item != history()->first());
    }
    return QString();

}

//
// changing a spinbox in klipper's config-dialog causes the lineedit-contents
// of the spinbox to be selected and hence the clipboard changes. But we don't
// want all those items in klipper's history. See #41917
//
bool Klipper::ignoreClipboardChanges() const
{
    QWidget *focusWidget = qApp->focusWidget();
    if ( focusWidget )
    {
        if ( focusWidget->inherits( "QSpinBox" ) ||
             (focusWidget->parentWidget() &&
              focusWidget->inherits("QLineEdit") &&
              focusWidget->parentWidget()->inherits("QSpinWidget")) )
        {
            return true;
        }
    }

    return false;
}

void Klipper::updateTimestamp()
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        QX11Info::setAppTime(QX11Info::getTimestamp());
    }
#endif
}

void Klipper::editData(const QSharedPointer< const HistoryItem > &item)
{
    QPointer<QDialog> dlg(new QDialog());
    dlg->setWindowTitle( i18n("Edit Contents") );
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, dlg.data(), &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dlg.data(), &QDialog::reject);
    connect(dlg.data(), &QDialog::finished, dlg.data(),
        [this, dlg, item](int result) {
            emit editFinished(item, result);
            dlg->deleteLater();
        }
    );

    KTextEdit *edit = new KTextEdit( dlg );
    edit->setAcceptRichText(false);
    if (item) {
        edit->setPlainText( item->text() );
    }
    edit->setFocus();
    edit->setMinimumSize( 300, 40 );
    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->addWidget(edit);
    layout->addWidget(buttons);
    dlg->adjustSize();

    connect(dlg.data(), &QDialog::accepted, this, [this, edit, item]() {
        QString text = edit->toPlainText();
        if (item) {
            m_history->remove( item );
        }
        m_history->insert(HistoryItemPtr(new HistoryStringItem(text)));
        if (m_myURLGrabber) {
            m_myURLGrabber->checkNewData(HistoryItemConstPtr(m_history->first()));
        }
    });

    if (m_mode == KlipperMode::Standalone) {
        dlg->setModal(true);
        dlg->exec();
    } else if (m_mode == KlipperMode::DataEngine) {
        dlg->open();
    }
}

class BarcodeLabel : public QLabel
{
public:
    BarcodeLabel(Prison::AbstractBarcode *barcode, QWidget *parent = nullptr)
        : QLabel(parent)
        , m_barcode(barcode)
        {
            setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            setPixmap(QPixmap::fromImage(m_barcode->toImage(size())));
        }
protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        setPixmap(QPixmap::fromImage(m_barcode->toImage(event->size())));
    }
private:
    QScopedPointer<Prison::AbstractBarcode> m_barcode;
};

void Klipper::showBarcode(const QSharedPointer< const HistoryItem > &item)
{
    using namespace Prison;
    QPointer<QDialog> dlg(new QDialog());
    dlg->setWindowTitle( i18n("Mobile Barcode") );
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, dlg);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, dlg.data(), &QDialog::accept);
    connect(dlg.data(), &QDialog::finished, dlg.data(), &QDialog::deleteLater);

    QWidget* mw = new QWidget(dlg);
    QHBoxLayout* layout = new QHBoxLayout(mw);

    {
        AbstractBarcode *qrCode = createBarcode(QRCode);
        if (qrCode) {
            if(item) {
                qrCode->setData(item->text());
            }
            BarcodeLabel *qrCodeLabel = new BarcodeLabel(qrCode, mw);
            layout->addWidget(qrCodeLabel);
        }
    }
    {
        AbstractBarcode *dataMatrix = createBarcode(DataMatrix);
        if (dataMatrix) {
            if (item) {
                dataMatrix->setData(item->text());
            }
            BarcodeLabel *dataMatrixLabel = new BarcodeLabel(dataMatrix, mw);
            layout->addWidget(dataMatrixLabel);
       }
    }

    mw->setFocus();
    QVBoxLayout *vBox = new QVBoxLayout(dlg);
    vBox->addWidget(mw);
    vBox->addWidget(buttons);
    dlg->adjustSize();

    if (m_mode == KlipperMode::Standalone) {
        dlg->setModal(true);
        dlg->exec();
    } else if (m_mode == KlipperMode::DataEngine) {
        dlg->open();
    }
}

void Klipper::slotAskClearHistory()
{
    int clearHist = KMessageBox::questionYesNo(nullptr,
                                               i18n("Really delete entire clipboard history?"),
                                               i18n("Delete clipboard history?"),
                                               KStandardGuiItem::yes(),
                                               KStandardGuiItem::no(),
                                               QStringLiteral("really_clear_history"),
                                               KMessageBox::Dangerous);
    if (clearHist == KMessageBox::Yes) {
      history()->slotClear();
      saveHistory();
    }

}

void Klipper::slotCycleNext()
{
    //do cycle and show popup only if we have something in clipboard
    if (m_history->first()) {
        m_history->cycleNext();
        emit passivePopup(i18n("Clipboard history"), cycleText());
    }
}

void Klipper::slotCyclePrev()
{
    //do cycle and show popup only if we have something in clipboard
    if (m_history->first()) {
        m_history->cyclePrev();
        emit passivePopup(i18n("Clipboard history"), cycleText());
    }
}

QString Klipper::cycleText() const
{
    const int WIDTH_IN_PIXEL = 400;

    auto itemprev = m_history->prevInCycle();
    auto item = m_history->first();
    auto itemnext = m_history->nextInCycle();

    QFontMetrics font_metrics(m_popup->fontMetrics());
    QString result(QStringLiteral("<table>"));

    if (itemprev) {
        result += QLatin1String("<tr><td>");
        result += i18n("up");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemprev->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("<tr><td>");
    result += i18n("current");
    result += QLatin1String("</td><td><b>");
    result += font_metrics.elidedText(item->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
    result += QLatin1String("</b></td></tr>");

    if (itemnext) {
        result += QLatin1String("<tr><td>");
        result += i18n("down");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemnext->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("</table>");
    return result;
}

