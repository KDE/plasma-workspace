/*

    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipper.h"

#include "klipper_debug.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDBusConnection>
#include <QDialog>
#include <QDir>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>

#include <KAboutData>
#include <KActionCollection>
#include <KGlobalAccel>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>
#include <KToggleAction>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include "../c_ptr.h"
#include "configdialog.h"
#include "historycycler.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipperpopup.h"
#include "klippersettings.h"
#include "systemclipboard.h"

#include <Prison/Barcode>

#include <config-X11.h>
#if HAVE_X11
#include <private/qtx11extras_p.h>
#include <xcb/xcb.h>
#endif

std::shared_ptr<Klipper> Klipper::self()
{
    static std::weak_ptr<Klipper> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<Klipper> ptr = std::make_shared<Klipper>(nullptr, KSharedConfig::openConfig(QStringLiteral("klipperrc")));
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

// config == KGlobal::config for process, otherwise applet
Klipper::Klipper(QObject *parent, const KSharedConfigPtr &config)
    : QObject(parent)
    , m_clip(SystemClipboard::self())
    , m_historyCycler(new HistoryCycler(this))
    , m_quitAction(nullptr)
    , m_config(config)
    , m_plasmashell(nullptr)
{
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.klipper"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/klipper"),
                                                 this,
                                                 QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

    connect(m_clip.get(), &SystemClipboard::ignored, this, &Klipper::slotIgnored);
    connect(m_clip.get(), &SystemClipboard::newClipData, this, &Klipper::checkClipData);

    m_historyModel = HistoryModel::self();
    m_popup = std::make_unique<KlipperPopup>();
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::slotHistoryChanged);
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::clipboardHistoryUpdated);

    // we need that collection, otherwise KToggleAction is not happy :}
    m_collection = new KActionCollection(this);

    m_toggleURLGrabAction = new KToggleAction(this);
    m_collection->addAction(QStringLiteral("clipboard_action"), m_toggleURLGrabAction);
    m_toggleURLGrabAction->setText(i18nc("@action:inmenu Toggle automatic action", "Automatic Action Popup Menu"));
    KGlobalAccel::setGlobalShortcut(m_toggleURLGrabAction, QKeySequence(Qt::META | Qt::CTRL | Qt::Key_X));
    connect(m_toggleURLGrabAction, &QAction::toggled, this, &Klipper::setURLGrabberEnabled);

    /*
     * Create URL grabber
     */
    m_myURLGrabber = new URLGrabber(this);
    connect(m_myURLGrabber, &URLGrabber::sigPopup, this, &Klipper::showPopupMenu);
    connect(m_myURLGrabber, &URLGrabber::sigDisablePopup, this, &Klipper::disableURLGrabber);
    connect(m_historyModel.get(), &HistoryModel::actionInvoked, m_myURLGrabber, &URLGrabber::invokeAction);

    /*
     * Load configuration settings
     */
    loadSettings();

    m_clearHistoryAction = m_collection->addAction(QStringLiteral("clear-history"));
    m_clearHistoryAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    m_clearHistoryAction->setText(i18nc("@action:inmenu", "C&lear Clipboard History"));
    KGlobalAccel::setGlobalShortcut(m_clearHistoryAction, QKeySequence());
    connect(m_clearHistoryAction, &QAction::triggered, m_historyModel.get(), &HistoryModel::clearHistory);

    QString CONFIGURE = QStringLiteral("configure");
    m_configureAction = m_collection->addAction(CONFIGURE);
    m_configureAction->setIcon(QIcon::fromTheme(CONFIGURE));
    m_configureAction->setText(i18nc("@action:inmenu", "&Configure Klipper…"));
    connect(m_configureAction, &QAction::triggered, this, &Klipper::slotConfigure);

    m_repeatAction = m_collection->addAction(QStringLiteral("repeat_action"));
    m_repeatAction->setText(i18nc("@action:inmenu", "Manually Invoke Action on Current Clipboard"));
    m_repeatAction->setIcon(QIcon::fromTheme(QStringLiteral("open-menu-symbolic")));
    KGlobalAccel::setGlobalShortcut(m_repeatAction, QKeySequence());
    connect(m_repeatAction, &QAction::triggered, this, &Klipper::slotRepeatAction);

    // add barcode for mobile phones
    m_showBarcodeAction = m_collection->addAction(QStringLiteral("show-barcode"));
    m_showBarcodeAction->setText(i18nc("@action:inmenu", "&Show Barcode…"));
    m_showBarcodeAction->setIcon(QIcon::fromTheme(QStringLiteral("view-barcode-qr")));
    KGlobalAccel::setGlobalShortcut(m_showBarcodeAction, QKeySequence());
    connect(m_showBarcodeAction, &QAction::triggered, this, [this]() {
        showBarcode(m_historyModel->first());
    });

    // Cycle through history
    m_cycleNextAction = m_collection->addAction(QStringLiteral("cycleNextAction"));
    m_cycleNextAction->setText(i18nc("@action:inmenu", "Next History Item"));
    m_cycleNextAction->setIcon(QIcon::fromTheme(QStringLiteral("go-next")));
    KGlobalAccel::setGlobalShortcut(m_cycleNextAction, QKeySequence());
    connect(m_cycleNextAction, &QAction::triggered, this, &Klipper::slotCycleNext);
    m_cyclePrevAction = m_collection->addAction(QStringLiteral("cyclePrevAction"));
    m_cyclePrevAction->setText(i18nc("@action:inmenu", "Previous History Item"));
    m_cyclePrevAction->setIcon(QIcon::fromTheme(QStringLiteral("go-previous")));
    KGlobalAccel::setGlobalShortcut(m_cyclePrevAction, QKeySequence());
    connect(m_cyclePrevAction, &QAction::triggered, this, &Klipper::slotCyclePrev);

    // Action to show items popup on mouse position
    m_showOnMousePos = m_collection->addAction(QStringLiteral("show-on-mouse-pos"));
    m_showOnMousePos->setText(i18nc("@action:inmenu", "Show Clipboard Items at Mouse Position"));
    m_showOnMousePos->setIcon(QIcon::fromTheme(QStringLiteral("view-list-text")));
    KGlobalAccel::setGlobalShortcut(m_showOnMousePos, QKeySequence(Qt::META | Qt::Key_V));
    connect(m_showOnMousePos, &QAction::triggered, this, &Klipper::slotPopupMenu);

    connect(this, &Klipper::passivePopup, this, [this](const QString &caption, const QString &text) {
        if (m_notification) {
            m_notification->setTitle(caption);
            m_notification->setText(text);
        } else {
            m_notification = KNotification::event(KNotification::Notification, caption, text, QStringLiteral("klipper"));
            // When Klipper is run as part of plasma, we still need to pretend to be it for notification settings to work
            m_notification->setHint(QStringLiteral("desktop-entry"), QStringLiteral("org.kde.klipper"));
        }
    });

    if (KWindowSystem::isPlatformWayland()) {
        auto registry = new KWayland::Client::Registry(this);
        auto connection = KWayland::Client::ConnectionThread::fromApplication(qGuiApp);
        connect(registry, &KWayland::Client::Registry::plasmaShellAnnounced, this, [registry, this](quint32 name, quint32 version) {
            if (!m_plasmashell) {
                m_plasmashell = registry->createPlasmaShell(name, version);
                m_popup->setPlasmaShell(m_plasmashell);
            }
        });
        connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, registry, [registry] {
            delete registry; // Avoid freeing resource when gui is deleted
        });
        registry->create(connection);
        registry->setup();
    }
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
    m_popup->show();
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
    updateTimestamp();
    auto item = HistoryItem::create(s);
    m_clip->setMimeData(item, SystemClipboard::SelectionMode(SystemClipboard::Clipboard | SystemClipboard::Selection));
    m_historyModel->insert(item);
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardContents()
{
    updateTimestamp();
    m_clip->clear();
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardHistory()
{
    updateTimestamp();
    m_historyModel->clear();
    saveSession();
}

// DBUS - don't call from Klipper itself
void Klipper::saveClipboardHistory()
{
    m_historyModel->saveHistory();
}

void Klipper::slotStartShowTimer()
{
    m_showTimer.start();
}

void Klipper::loadSettings()
{
    m_bKeepContents = KlipperSettings::keepClipboardContents();
    m_bReplayActionInHistory = KlipperSettings::replayActionInHistory();
    m_bNoNullClipboard = KlipperSettings::preventEmptyClipboard();
    if (m_bNoNullClipboard) {
        connect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &Klipper::slotReceivedEmptyClipboard, Qt::UniqueConnection);
    } else {
        disconnect(m_clip.get(), &SystemClipboard::receivedEmptyClipboard, this, &Klipper::slotReceivedEmptyClipboard);
    }
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

    m_historyModel->loadSettings();
}

void Klipper::saveSettings() const
{
    m_myURLGrabber->saveSettings();
    KlipperSettings::self()->setVersion(QStringLiteral(KLIPPER_VERSION_STRING));
    KlipperSettings::self()->save();

    // other settings should be saved automatically by KConfigDialog
}

void Klipper::showPopupMenu(QMenu *menu)
{
    Q_ASSERT(menu != nullptr);
    if (m_plasmashell) {
        menu->hide();
    }
    menu->popup(QCursor::pos());
    if (m_plasmashell) {
        menu->windowHandle()->installEventFilter(this);
    }
}

bool Klipper::eventFilter(QObject *filtered, QEvent *event)
{
    const bool ret = QObject::eventFilter(filtered, event);
    auto menuWindow = qobject_cast<QWindow *>(filtered);
    if (menuWindow && event->type() == QEvent::Expose && menuWindow->isVisible()) {
        auto surface = KWayland::Client::Surface::fromWindow(menuWindow);
        auto plasmaSurface = m_plasmashell->createSurface(surface, menuWindow);
        plasmaSurface->openUnderCursor();
        plasmaSurface->setSkipTaskbar(true);
        plasmaSurface->setSkipSwitcher(true);
        plasmaSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::AppletPopup);
        menuWindow->removeEventFilter(this);
    }
    return ret;
}

// save session on shutdown. Don't simply use the c'tor, as that may not be called.
void Klipper::saveSession()
{
    m_historyModel->saveHistory();
    saveSettings();
}

void Klipper::disableURLGrabber()
{
    QMessageBox *message = new QMessageBox(QMessageBox::Information,
                                           QString(),
                                           xi18nc("@info",
                                                  "You can enable URL actions later in the "
                                                  "<interface>Actions</interface> page of the "
                                                  "Clipboard applet's configuration window"));
    message->setAttribute(Qt::WA_DeleteOnClose);
    message->setModal(false);
    message->show();

    setURLGrabberEnabled(false);
}

void Klipper::slotConfigure()
{
    if (KConfigDialog::showDialog(QStringLiteral("preferences"))) {
        // This will never happen, because of the WA_DeleteOnClose below.
        return;
    }

    ConfigDialog *dlg = new ConfigDialog(nullptr, KlipperSettings::self(), this, m_collection);
    QMetaObject::invokeMethod(dlg, "setHelp", Qt::DirectConnection, Q_ARG(QString, QString::fromLatin1("preferences")));
    // This is necessary to ensure that the dialog is recreated
    // and therefore the controls are initialised from the current
    // Klipper settings every time that it is shown.
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &KConfigDialog::settingsChanged, this, [this]() {
        const bool bKeepContents_old = m_bKeepContents; // back up old value
        loadSettings();

        // BUG: 142882
        // Security: If user has save clipboard turned off, old data should be deleted from disk
        if (bKeepContents_old != m_bKeepContents) { // keepContents changed
            m_historyModel->saveHistory(!m_bKeepContents); // save history, empty = !keep
        }
    });
    dlg->show();
}

void Klipper::slotIgnored(QClipboard::Mode mode)
{
    // internal to klipper, ignoring QSpinBox selections
    // keep our old clipboard, thanks
    // This won't quite work, but it's close enough for now.
    // The trouble is that the top selection =! top clipboard
    // but we don't track that yet. We will....
    if (auto top = m_historyModel->first()) {
        m_clip->setMimeData(top, mode == QClipboard::Selection ? SystemClipboard::Selection : SystemClipboard::Clipboard);
    }
}

void Klipper::slotReceivedEmptyClipboard(QClipboard::Mode mode)
{
    Q_ASSERT(m_bNoNullClipboard);
    if (auto top = m_historyModel->first()) {
        // keep old clipboard after someone set it to null
        qCDebug(KLIPPER_LOG) << "Resetting clipboard (Prevent empty clipboard)";
        m_clip->setMimeData(top,
                            mode == QClipboard::Selection ? SystemClipboard::Selection : SystemClipboard::Clipboard,
                            SystemClipboard::ClipboardUpdateReason::PreventEmptyClipboard);
    }
}

void Klipper::slotPopupMenu()
{
    m_popup->show();
}

void Klipper::slotRepeatAction()
{
    auto top = m_historyModel->first();
    if (top && top->type() == HistoryItemType::Text) {
        m_myURLGrabber->invokeAction(top);
    }
}

void Klipper::setURLGrabberEnabled(bool enable)
{
    if (enable != m_bURLGrabber) {
        m_bURLGrabber = enable;
        m_lastURLGrabberTextSelection.clear();
        m_lastURLGrabberTextClipboard.clear();
        KlipperSettings::setURLGrabberEnabled(enable);
    }

    m_toggleURLGrabAction->setChecked(enable);

    // make it update its settings
    m_myURLGrabber->loadSettings();
}

void Klipper::slotHistoryChanged(bool isTop)
{
    if (!isTop || m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    if (m_bReplayActionInHistory && m_bURLGrabber) {
        slotRepeatAction();
    }
}

HistoryItemPtr Klipper::applyClipChanges(const QMimeData *clipData)
{
    Q_ASSERT(m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard));
    if (m_historyModel->rowCount() > 0) {
        if (m_bIgnoreImages && m_historyModel->first()->type() == HistoryItemType::Image) {
            m_historyModel->remove(m_historyModel->first()->uuid());
        }
    }

    HistoryItemPtr item = HistoryItem::create(clipData);

    bool saveToHistory = true;
    if (clipData->data(QStringLiteral("x-kde-passwordManagerHint")) == QByteArrayLiteral("secret")) {
        saveToHistory = false;
    }
    if (saveToHistory && item) {
        m_historyModel->insert(item);
    }

    return item;
}

void Klipper::checkClipData(QClipboard::Mode mode, const QMimeData *data)
{
    Q_ASSERT(m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard));
    bool changed = true; // ### FIXME (only relevant under polling, might be better to simply remove polling and rely on XFixes)

    // this must be below the "bNoNullClipboard" handling code!
    // XXX: I want a better handling of selection/clipboard in general.
    // XXX: Order sensitive code. Must die.
    const bool selectionMode = mode == QClipboard::Selection;
    if (selectionMode && m_bIgnoreSelection) {
        if (m_bSynchronize) {
            auto item = HistoryItem::create(data);
            if (item) [[likely]] { // applyClipChanges can return nullptr
                m_clip->setMimeData(item, SystemClipboard::Clipboard, SystemClipboard::ClipboardUpdateReason::SyncSelection);
            }
        }
        return;
    }

    if (selectionMode && m_bSelectionTextOnly && !data->hasText())
        return;

    if (m_bIgnoreImages && data->hasImage() && !data->hasText() /*BUG 491488*/ && !data->hasFormat(QStringLiteral("x-kde-force-image-copy"))) {
        return;
    }

    HistoryItemPtr item = applyClipChanges(data);
    if (changed) {
        qCDebug(KLIPPER_LOG) << "Synchronize?" << m_bSynchronize;
        if (m_bSynchronize && item) { // applyClipChanges can return nullptr
            m_clip->setMimeData(item, mode == QClipboard::Selection ? SystemClipboard::Clipboard : SystemClipboard::Selection);
        }
    }
    QString &lastURLGrabberText = selectionMode ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if (m_bURLGrabber && item && data->hasText()) {
        m_myURLGrabber->checkNewData(std::const_pointer_cast<const HistoryItem>(item));

        // Make sure URLGrabber doesn't repeat all the time if klipper reads the same
        // text all the time (e.g. because XFixes is not available and the application
        // has broken TIMESTAMP target). Using most recent history item may not always
        // work.
        if (item->text() != lastURLGrabberText) {
            lastURLGrabberText = item->text();
        }
    } else {
        lastURLGrabberText.clear();
    }
}

QStringList Klipper::getClipboardHistoryMenu()
{
    QStringList menu;
    for (int i = 0, count = m_historyModel->rowCount(); i < count; ++i) {
        menu.emplace_back(m_historyModel->index(i).data(Qt::DisplayRole).toString());
    }

    return menu;
}

QString Klipper::getClipboardHistoryItem(int i)
{
    return m_historyModel->index(i).data(Qt::DisplayRole).toString();
}

//
// changing a spinbox in klipper's config-dialog causes the lineedit-contents
// of the spinbox to be selected and hence the clipboard changes. But we don't
// want all those items in klipper's history. See #41917
//
bool Klipper::ignoreClipboardChanges() const
{
    QWidget *focusWidget = qApp->focusWidget();
    if (focusWidget) {
        if (focusWidget->inherits("QSpinBox")
            || (focusWidget->parentWidget() && focusWidget->inherits("QLineEdit") && focusWidget->parentWidget()->inherits("QSpinWidget"))) {
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

class BarcodeLabel : public QLabel
{
public:
    BarcodeLabel(Prison::Barcode &&barcode, QWidget *parent = nullptr)
        : QLabel(parent)
        , m_barcode(std::move(barcode))
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        setPixmap(QPixmap::fromImage(m_barcode.toImage(size())));
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QLabel::resizeEvent(event);
        setPixmap(QPixmap::fromImage(m_barcode.toImage(event->size())));
    }

private:
    Prison::Barcode m_barcode;
};

void Klipper::showBarcode(std::shared_ptr<const HistoryItem> item)
{
    QPointer<QDialog> dlg(new QDialog());
    dlg->setWindowTitle(i18n("Mobile Barcode"));
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, dlg);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, dlg.data(), &QDialog::accept);
    connect(dlg.data(), &QDialog::finished, dlg.data(), &QDialog::deleteLater);

    QWidget *mw = new QWidget(dlg);
    QHBoxLayout *layout = new QHBoxLayout(mw);

    {
        auto qrCode = Prison::Barcode::create(Prison::QRCode);
        if (qrCode) {
            if (item) {
                qrCode->setData(item->text());
            }
            BarcodeLabel *qrCodeLabel = new BarcodeLabel(std::move(*qrCode), mw);
            layout->addWidget(qrCodeLabel);
        }
    }
    {
        auto dataMatrix = Prison::Barcode::create(Prison::DataMatrix);
        if (dataMatrix) {
            if (item) {
                dataMatrix->setData(item->text());
            }
            BarcodeLabel *dataMatrixLabel = new BarcodeLabel(std::move(*dataMatrix), mw);
            layout->addWidget(dataMatrixLabel);
        }
    }

    mw->setFocus();
    QVBoxLayout *vBox = new QVBoxLayout(dlg);
    vBox->addWidget(mw);
    vBox->addWidget(buttons);
    dlg->adjustSize();
    dlg->open();
}

void Klipper::slotCycleNext()
{
    // do cycle and show popup only if we have something in clipboard
    if (m_historyModel->first()) {
        m_historyCycler->cycleNext();
        Q_EMIT passivePopup(i18n("Clipboard history"), cycleText());
    }
}

void Klipper::slotCyclePrev()
{
    // do cycle and show popup only if we have something in clipboard
    if (m_historyModel->first()) {
        m_historyCycler->cyclePrev();
        Q_EMIT passivePopup(i18n("Clipboard history"), cycleText());
    }
}

QString Klipper::cycleText() const
{
    const int WIDTH_IN_PIXEL = 400;

    auto itemPrev = m_historyCycler->prevInCycle();
    auto item = m_historyModel->first();
    auto itemNext = m_historyCycler->nextInCycle();

    QFontMetrics font_metrics(QWidget().fontMetrics());
    QString result(QStringLiteral("<table>"));

    if (itemPrev) {
        result += QLatin1String("<tr><td>");
        result += i18n("up");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemPrev->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("<tr><td>");
    result += i18n("current");
    result += QLatin1String("</td><td><b>");
    result += font_metrics.elidedText(item->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
    result += QLatin1String("</b></td></tr>");

    if (itemNext) {
        result += QLatin1String("<tr><td>");
        result += i18n("down");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemNext->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("</table>");
    return result;
}
