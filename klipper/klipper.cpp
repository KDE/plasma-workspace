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

#include <QMenu>
#include <QDBusConnection>

#include <k4aboutdata.h>
#include <KGlobalAccel>
#include <KLocale>
#include <KMessageBox>
#include <KSaveFile>
#include <KSessionManager>
#include <KStandardDirs>
#include <KDebug>
#include <KDialog>
#include <KGlobalSettings>
#include <KActionCollection>
#include <KToggleAction>
#include <KTextEdit>
#include <KApplication>

#include "configdialog.h"
#include "klippersettings.h"
#include "urlgrabber.h"
#include "version.h"
#include "history.h"
#include "historyitem.h"
#include "historystringitem.h"
#include "klipperpopup.h"

#ifdef HAVE_PRISON
#include <prison/BarcodeWidget>
#include <prison/DataMatrixBarcode>
#include <prison/QRCodeBarcode>
#endif

#if HAVE_X11
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#endif

//#define NOISY_KLIPPER

namespace {
    /**
     * Use this when manipulating the clipboard
     * from within clipboard-related signals.
     *
     * This avoids issues such as mouse-selections that immediately
     * disappear.
     * pattern: Resource Acqusition is Initialisation (RAII)
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

/**
 * Helper class to save history upon session exit.
 */
class KlipperSessionManager : public KSessionManager
{
public:
    KlipperSessionManager( Klipper* k )
        : klipper( k )
        {}

    virtual ~KlipperSessionManager() {}

    /**
     * Save state upon session exit.
     *
     * Saving history on session save
     */
    virtual bool commitData( QSessionManager& ) {
        klipper->saveSession();
        return true;
    }
private:
    Klipper* klipper;
};

// config == KGlobal::config for process, otherwise applet
Klipper::Klipper(QObject* parent, const KSharedConfigPtr& config)
    : QObject( parent )
    , m_overflowCounter( 0 )
    , m_locklevel( 0 )
    , m_config( config )
    , m_pendingContentsCheck( false )
    , m_sessionManager( new KlipperSessionManager( this ))
{
    setenv("KSNI_NO_DBUSMENU", "1", 1);
    QDBusConnection::sessionBus().registerObject("/klipper", this, QDBusConnection::ExportScriptableSlots);

    updateTimestamp(); // read initial X user time
    m_clip = kapp->clipboard();

    connect( m_clip, SIGNAL(changed(QClipboard::Mode)),
             this, SLOT(newClipData(QClipboard::Mode)) );

    connect( &m_overflowClearTimer, SIGNAL(timeout()), SLOT(slotClearOverflow()));

    m_pendingCheckTimer.setSingleShot( true );
    connect( &m_pendingCheckTimer, SIGNAL(timeout()), SLOT(slotCheckPending()));


    m_history = new History( this );

    // we need that collection, otherwise KToggleAction is not happy :}
    m_collection = new KActionCollection( this );

    m_toggleURLGrabAction = new KToggleAction( this );
    m_collection->addAction( "clipboard_action", m_toggleURLGrabAction );
    m_toggleURLGrabAction->setText(i18n("Enable Clipboard Actions"));
    KGlobalAccel::self()->setShortcut(m_toggleURLGrabAction, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_X));
    connect( m_toggleURLGrabAction, SIGNAL(toggled(bool)),
             this, SLOT(setURLGrabberEnabled(bool)));

    /*
     * Create URL grabber
     */
    m_myURLGrabber = new URLGrabber(m_history);
    connect( m_myURLGrabber, SIGNAL(sigPopup(QMenu*)),
            SLOT(showPopupMenu(QMenu*)) );
    connect( m_myURLGrabber, SIGNAL(sigDisablePopup()),
            SLOT(disableURLGrabber()) );

    /*
     * Load configuration settings
     */
    loadSettings();

    // load previous history if configured
    if (m_bKeepContents) {
        loadHistory();
    }

    m_clearHistoryAction = m_collection->addAction( "clear-history" );
    m_clearHistoryAction->setIcon( QIcon::fromTheme("edit-clear-history") );
    m_clearHistoryAction->setText( i18n("C&lear Clipboard History") );
    KGlobalAccel::self()->setShortcut(m_clearHistoryAction, QList<QKeySequence>());
    connect(m_clearHistoryAction, SIGNAL(triggered()), SLOT(slotAskClearHistory()));

    m_configureAction = m_collection->addAction( "configure" );
    m_configureAction->setIcon( QIcon::fromTheme("configure") );
    m_configureAction->setText( i18n("&Configure Klipper...") );
    connect(m_configureAction, SIGNAL(triggered(bool)), SLOT(slotConfigure()));

    m_quitAction = m_collection->addAction( "quit" );
    m_quitAction->setIcon( QIcon::fromTheme("application-exit") );
    m_quitAction->setText( i18nc("@item:inmenu Quit Klipper", "&Quit") );
    connect(m_quitAction, SIGNAL(triggered(bool)), SLOT(slotQuit()));

    m_repeatAction = m_collection->addAction("repeat_action");
    m_repeatAction->setText(i18n("Manually Invoke Action on Current Clipboard"));
    KGlobalAccel::self()->setShortcut(m_repeatAction, QList<QKeySequence>() << QKeySequence(Qt::ALT+Qt::CTRL+Qt::Key_R));
    connect(m_repeatAction, SIGNAL(triggered()), SLOT(slotRepeatAction()));

    // add an edit-possibility
    m_editAction = m_collection->addAction("edit_clipboard");
    m_editAction->setIcon(QIcon::fromTheme("document-properties"));
    m_editAction->setText(i18n("&Edit Contents..."));
    KGlobalAccel::self()->setShortcut(m_editAction, QList<QKeySequence>());
    connect(m_editAction, SIGNAL(triggered()), SLOT(slotEditData()));

#ifdef HAVE_PRISON
    // add barcode for mobile phones
    m_showBarcodeAction = m_collection->addAction("show-barcode");
    m_showBarcodeAction->setText(i18n("&Show Barcode..."));
    KGlobalAccel::self()->setShortcut(m_showBarcodeAction, QList<QKeySequence>());
    connect(m_showBarcodeAction, SIGNAL(triggered()), SLOT(slotShowBarcode()));
#endif

    // Cycle through history
    m_cycleNextAction = m_collection->addAction("cycleNextAction");
    m_cycleNextAction->setText(i18n("Next History Item"));
    KGlobalAccel::self()->setShortcut(m_cycleNextAction, QList<QKeySequence>());
    connect(m_cycleNextAction, SIGNAL(triggered(bool)), SLOT(slotCycleNext()));
    m_cyclePrevAction = m_collection->addAction("cyclePrevAction");
    m_cyclePrevAction->setText(i18n("Previous History Item"));
    KGlobalAccel::self()->setShortcut(m_cyclePrevAction, QList<QKeySequence>());
    connect(m_cyclePrevAction, SIGNAL(triggered(bool)), SLOT(slotCyclePrev()));

    // Action to show Klipper popup on mouse position
    m_showOnMousePos = m_collection->addAction("show-on-mouse-pos");
    m_showOnMousePos->setText(i18n("Open Klipper at Mouse Position"));
    KGlobalAccel::self()->setShortcut(m_showOnMousePos, QList<QKeySequence>());
    connect(m_showOnMousePos, SIGNAL(triggered(bool)), this, SLOT(slotPopupMenu()));

    KlipperPopup* popup = history()->popup();
    connect ( history(), SIGNAL(topChanged()), SLOT(slotHistoryTopChanged()) );
    connect( popup, SIGNAL(aboutToShow()), SLOT(slotStartShowTimer()) );

    popup->plugAction( m_toggleURLGrabAction );
    popup->plugAction( m_clearHistoryAction );
    popup->plugAction( m_configureAction );
    popup->plugAction( m_repeatAction );
    popup->plugAction( m_editAction );
#ifdef HAVE_PRISON
    popup->plugAction( m_showBarcodeAction );
#endif
    popup->plugAction( m_quitAction );
}

Klipper::~Klipper()
{
    delete m_sessionManager;
    delete m_myURLGrabber;
}

// DBUS
QString Klipper::getClipboardContents()
{
    return getClipboardHistoryItem(0);
}

void Klipper::showKlipperPopupMenu() {
    slotPopupMenu();
}
void Klipper::showKlipperManuallyInvokeActionMenu() {
    slotRepeatAction();
}


// DBUS - don't call from Klipper itself
void Klipper::setClipboardContents(QString s)
{
    if (s.isEmpty())
        return;
    Ignore lock( m_locklevel );
    updateTimestamp();
    HistoryStringItem* item = new HistoryStringItem( s );
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
    slotClearClipboard();
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
    // Convert 4.3 settings
    if (KlipperSettings::synchronize() != 3) {
      // 2 was the id of "Ignore selection" radiobutton
      m_bIgnoreSelection = KlipperSettings::synchronize() == 2;
      // 0 was the id of "Synchronize contents" radiobutton
      m_bSynchronize = KlipperSettings::synchronize() == 0;
      KConfigSkeletonItem* item = KlipperSettings::self()->findItem("SyncClipboards");
      item->setProperty(m_bSynchronize);
      item = KlipperSettings::self()->findItem("IgnoreSelection");
      item->setProperty(m_bIgnoreSelection);
      item =  KlipperSettings::self()->findItem("Synchronize"); // Mark property as converted.
      item->setProperty(3);
      KlipperSettings::self()->writeConfig();
      KlipperSettings::self()->readConfig();

    }
}

void Klipper::saveSettings() const
{
    m_myURLGrabber->saveSettings();
    KlipperSettings::self()->setVersion(klipper_version);
    KlipperSettings::self()->writeConfig();

    // other settings should be saved automatically by KConfigDialog
}

void Klipper::showPopupMenu( QMenu* menu )
{
    Q_ASSERT( menu != 0L );

    QSize size = menu->sizeHint(); // geometry is not valid until it's shown
    QPoint pos = QCursor::pos();
    // ### We can't know where the systray icon is (since it can be hidden or shown
    //     in several places), so the cursor position is the only option.

    if ( size.height() < pos.y() )
        pos.ry() -= size.height();

    menu->popup(pos);
}

bool Klipper::loadHistory() {
    static const char* const failed_load_warning =
        "Failed to load history resource. Clipboard history cannot be read.";
    // don't use "appdata", klipper is also a kicker applet
    QString history_file_name = KStandardDirs::locateLocal( "data", "klipper/history2.lst" );
    QFile history_file( history_file_name );
    if ( !history_file.exists() ) {
        kWarning() << failed_load_warning << ": " << "History file does not exist" ;
        return false;
    }
    if ( !history_file.open( QIODevice::ReadOnly ) ) {
        kWarning() << failed_load_warning << ": " << history_file.errorString() ;
        return false;
    }
    QDataStream file_stream( &history_file );
    if( file_stream.atEnd()) {
        kWarning() << failed_load_warning << ": " << "Error in reading data" ;
        return false;
    }
    QByteArray data;
    quint32 crc;
    file_stream >> crc >> data;
    if( crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() ) != crc ) {
        kWarning() << failed_load_warning << ": " << "CRC checksum does not match" ;
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
    QList<HistoryItem*> reverseList;
    for ( HistoryItem* item = HistoryItem::create( history_stream );
          item;
          item = HistoryItem::create( history_stream ) )
    {
        reverseList.prepend( item );
    }

    history()->slotClear();

    for ( QList<HistoryItem*>::const_iterator it = reverseList.constBegin();
          it != reverseList.constEnd();
          ++it )
    {
        history()->forceInsert( *it );
    }

    if ( !history()->empty() ) {
        setClipboard( *history()->first(), Clipboard | Selection );
    }

    return true;
}

void Klipper::saveHistory(bool empty) {
    static const char* const failed_save_warning =
        "Failed to save history. Clipboard history cannot be saved.";
    // don't use "appdata", klipper is also a kicker applet
    QString history_file_name( KStandardDirs::locateLocal( "data", "klipper/history2.lst" ) );
    if ( history_file_name.isNull() || history_file_name.isEmpty() ) {
        kWarning() << failed_save_warning ;
        return;
    }
    KSaveFile history_file( history_file_name );
    if ( !history_file.open() ) {
        kWarning() << failed_save_warning ;
        return;
    }
    QByteArray data;
    QDataStream history_stream( &data, QIODevice::WriteOnly );
    history_stream << klipper_version; // const char*

    if (!empty) {
        const HistoryItem *item = history()->first();
        if (item) {
            do {
                history_stream << item;
                item = history()->find(item->next_uuid());
            } while (item != history()->first());
        }
    }

    quint32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    QDataStream ds ( &history_file );
    ds << crc << data;
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
    KMessageBox::information( 0L,
                              i18n( "You can enable URL actions later by left-clicking on the "
                                    "Klipper icon and selecting 'Enable Clipboard Actions'" ) );

    setURLGrabberEnabled( false );
}

void Klipper::slotConfigure()
{
    if (KConfigDialog::showDialog("preferences")) {
        return;
    }

    ConfigDialog *dlg = new ConfigDialog( 0, KlipperSettings::self(), this, m_collection );
    connect(dlg, SIGNAL(settingsChanged(QString)), SLOT(loadSettings()));

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
    int autoStart = KMessageBox::questionYesNoCancel(0, i18n("Should Klipper start automatically when you login?"),
                                                     i18n("Automatically Start Klipper?"), KGuiItem(i18n("Start")),
                                                     KGuiItem(i18n("Do Not Start")), KStandardGuiItem::cancel(), "StartAutomatically");

    KConfigGroup config( KSharedConfig::openConfig(), "General");
    if ( autoStart == KMessageBox::Yes ) {
        config.writeEntry("AutoStart", true);
    } else if ( autoStart == KMessageBox::No) {
        config.writeEntry("AutoStart", false);
    } else  // cancel chosen don't quit
        return;
    config.sync();

    kapp->quit();

}

void Klipper::slotPopupMenu() {
    KlipperPopup* popup = history()->popup();
    popup->ensureClean();
    popup->slotSetTopActive();
    showPopupMenu( popup );
}


void Klipper::slotRepeatAction()
{
    const HistoryStringItem* top = dynamic_cast<const HistoryStringItem*>( history()->first() );
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

    const HistoryItem* topitem = history()->first();
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

HistoryItem* Klipper::applyClipChanges( const QMimeData* clipData )
{
    if ( m_locklevel ) {
        return 0L;
    }
    Ignore lock( m_locklevel );
    HistoryItem* item = HistoryItem::create( clipData );
    history()->insert( item );
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
    Window root, child;
    int root_x, root_y, win_x, win_y;
    uint state;
    XQueryPointer( QX11Info::display(), QX11Info::appRootWindow(), &root, &child,
                   &root_x, &root_y, &win_x, &win_y, &state );
    if( ( state & ( ShiftMask | Button1Mask )) == ShiftMask // #85198
        || ( state & Button1Mask ) == Button1Mask ) { // #80302
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
        const HistoryItem* top = history()->first();
        if ( top ) {
            setClipboard( *top, selectionMode ? Selection : Clipboard);
        }
        return;
    }

// debug code
#ifdef NOISY_KLIPPER
    kDebug() << "Checking clip data";

    kDebug() << "====== c h e c k C l i p D a t a ============================"
              << kBacktrace()
              << "====== c h e c k C l i p D a t a ============================"
              << endl;;


    if ( sender() ) {
        kDebug() << "sender=" << sender()->objectName();
    } else {
        kDebug() << "no sender";
    }

    kDebug() << "\nselectionMode=" << selectionMode
              << "\nowning (sel,cli)=(" << m_clip->ownsSelection() << "," << m_clip->ownsClipboard() << ")"
              << "\ntext=" << m_clip->text( selectionMode ? QClipboard::Selection : QClipboard::Clipboard) << endl;
#endif

    const QMimeData* data = m_clip->mimeData( selectionMode ? QClipboard::Selection : QClipboard::Clipboard );
    if ( !data ) {
        kWarning() << "No data in clipboard. This not not supposed to happen.";
        return;
    }

    bool changed = true; // ### FIXME (only relevant under polling, might be better to simply remove polling and rely on XFixes)
    bool clipEmpty = data->formats().isEmpty();
    if (clipEmpty) {
        // Might be a timeout. Try again
        clipEmpty = data->formats().isEmpty();
#ifdef NOISY_KLIPPER
        kDebug() << "was empty. Retried, now " << (clipEmpty?" still empty":" no longer empty");
#endif
    }

    if ( changed && clipEmpty && m_bNoNullClipboard ) {
        const HistoryItem* top = history()->first();
        if ( top ) {
            // keep old clipboard after someone set it to null
#ifdef NOISY_KLIPPER
            kDebug() << "Resetting clipboard (Prevent empty clipboard)";
#endif
            setClipboard( *top, selectionMode ? Selection : Clipboard );
        }
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
        if( m_bIgnoreImages )
            return;
    }
    else // unknown, ignore
        return;

    HistoryItem* item = applyClipChanges( data );
    if (changed) {
#ifdef NOISY_KLIPPER
        kDebug() << "Synchronize?" << m_bSynchronize;
#endif
        if ( m_bSynchronize && item ) {
            setClipboard( *item, selectionMode ? Clipboard : Selection );
        }
    }
    QString& lastURLGrabberText = selectionMode
        ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if( m_bURLGrabber && item && data->hasText())
    {
        m_myURLGrabber->checkNewData( item );

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

void Klipper::setClipboard( const HistoryItem& item, int mode )
{
    Ignore lock( m_locklevel );

    Q_ASSERT( ( mode & 1 ) == 0 ); // Warn if trying to pass a boolean as a mode.

    if ( mode & Selection ) {
#ifdef NOISY_KLIPPER
        kDebug() << "Setting selection to <" << item.text() << ">";
#endif
        m_clip->setMimeData( item.mimeData(), QClipboard::Selection );
    }
    if ( mode & Clipboard ) {
#ifdef NOISY_KLIPPER
        kDebug() << "Setting clipboard to <" << item.text() << ">";
#endif
        m_clip->setMimeData( item.mimeData(), QClipboard::Clipboard );
    }

}

void Klipper::slotClearOverflow()
{
    m_overflowClearTimer.stop();

    if( m_overflowCounter > MAX_CLIPBOARD_CHANGES ) {
        kDebug() << "App owning the clipboard/selection is lame";
        // update to the latest data - this unfortunately may trigger the problem again
        newClipData( QClipboard::Selection ); // Always the selection.
    }
    m_overflowCounter = 0;
}

QStringList Klipper::getClipboardHistoryMenu()
{
    QStringList menu;
    const HistoryItem* item = history()->first();
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
    const HistoryItem* item = history()->first();
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
    QX11Info::setAppTime(QX11Info::getTimestamp());
}

static const char * const description =
      I18N_NOOP("KDE cut & paste history utility");

void Klipper::createAboutData()
{
  m_about_data = new K4AboutData("klipper", 0, ki18n("Klipper"),
    klipper_version, ki18n(description), K4AboutData::License_GPL,
		       ki18n("(c) 1998, Andrew Stanley-Jones\n"
		       "1998-2002, Carsten Pfeiffer\n"
		       "2001, Patrick Dubroy"));

  m_about_data->addAuthor(ki18n("Carsten Pfeiffer"),
                      ki18n("Author"),
                      "pfeiffer@kde.org");

  m_about_data->addAuthor(ki18n("Andrew Stanley-Jones"),
                      ki18n( "Original Author" ),
                      "asj@cban.com");

  m_about_data->addAuthor(ki18n("Patrick Dubroy"),
                      ki18n("Contributor"),
                      "patrickdu@corel.com");

  m_about_data->addAuthor( ki18n("Luboš Luňák"),
                      ki18n("Bugfixes and optimizations"),
                      "l.lunak@kde.org");

  m_about_data->addAuthor( ki18n("Esben Mose Hansen"),
                      ki18n("Maintainer"),
                      "kde@mosehansen.dk");
}

void Klipper::destroyAboutData()
{
  delete m_about_data;
  m_about_data = NULL;
}

K4AboutData* Klipper::m_about_data;

K4AboutData* Klipper::aboutData()
{
  return m_about_data;
}

void Klipper::slotEditData()
{
    const HistoryStringItem* item = dynamic_cast<const HistoryStringItem*>(m_history->first());

    KDialog dlg;
    dlg.setModal( true );
    dlg.setCaption( i18n("Edit Contents") );
    dlg.setButtons( KDialog::Ok | KDialog::Cancel );

    KTextEdit *edit = new KTextEdit( &dlg );
    if (item) {
        edit->setText( item->text() );
    }
    edit->setFocus();
    edit->setMinimumSize( 300, 40 );
    dlg.setMainWidget( edit );
    dlg.adjustSize();

    if ( dlg.exec() == KDialog::Accepted ) {
        QString text = edit->toPlainText();
        if (item) {
            m_history->remove( item );
        }
        m_history->insert( new HistoryStringItem(text) );
        if (m_myURLGrabber) {
            m_myURLGrabber->checkNewData( m_history->first() );
        }
    }

}

#ifdef HAVE_PRISON
void Klipper::slotShowBarcode()
{
    using namespace prison;
    const HistoryStringItem* item = dynamic_cast<const HistoryStringItem*>(m_history->first());

    KDialog dlg;
    dlg.setModal( true );
    dlg.setCaption( i18n("Mobile Barcode") );
    dlg.setButtons( KDialog::Ok );

    QWidget* mw = new QWidget(&dlg);
    QHBoxLayout* layout = new QHBoxLayout(mw);

    BarcodeWidget* qrcode = new BarcodeWidget(new QRCodeBarcode());
    BarcodeWidget* datamatrix = new BarcodeWidget(new DataMatrixBarcode());

    if (item) {
        qrcode->setData( item->text() );
        datamatrix->setData( item->text() );
    }

    layout->addWidget(qrcode);
    layout->addWidget(datamatrix);

    mw->setFocus();
    dlg.setMainWidget( mw );
    dlg.adjustSize();

    dlg.exec();
}
#endif //HAVE_PRISON

void Klipper::slotAskClearHistory()
{
    int clearHist = KMessageBox::questionYesNo(0,
                                               i18n("Really delete entire clipboard history?"),
                                               i18n("Delete clipboard history?"),
                                               KStandardGuiItem::yes(),
                                               KStandardGuiItem::no(),
                                               QString::fromUtf8("really_clear_history"),
                                               KMessageBox::Dangerous);
    if (clearHist == KMessageBox::Yes) {
      history()->slotClear();
      slotClearClipboard();
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

    const HistoryItem* itemprev = m_history->prevInCycle();
    const HistoryItem* item = m_history->first();
    const HistoryItem* itemnext = m_history->nextInCycle();

    QFontMetrics font_metrics(m_history->popup()->fontMetrics());
    QString result("<table>");

    if (itemprev) {
        result += "<tr><td>";
        result += i18n("up");
        result += "</td><td>";
        result += font_metrics.elidedText(Qt::escape(itemprev->text().simplified()), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += "</td></tr>";
    }

    result += "<tr><td>";
    result += i18n("current");
    result += "</td><td><b>";
    result += font_metrics.elidedText(Qt::escape(item->text().simplified()), Qt::ElideMiddle, WIDTH_IN_PIXEL);
    result += "</b></td></tr>";

    if (itemnext) {
        result += "<tr><td>";
        result += i18n("down");
        result += "</td><td>";
        result += font_metrics.elidedText(Qt::escape(itemnext->text().simplified()), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += "</td></tr>";
    }

    result += "</table>";
    return result;
}

#include "klipper.moc"
