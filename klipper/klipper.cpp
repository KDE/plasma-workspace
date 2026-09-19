/*

    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipper.h"

#include "autopastehelpers.h"
#include "klipper_debug.h"
#include <QDBusConnection>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMimeData>
#include <QTimer>
#include <QWindow>
#if HAVE_X11
#include <QtGui/qguiapplication_platform.h>
#endif

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KNotification>
#include <KToggleAction>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/fakeinput.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>

#include <PlasmaQuick/PlasmaShellWaylandIntegration>

#include "configdialog.h"
#include "historycycler.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipperpopup.h"
#include "klippersettings.h"
#include "systemclipboard.h"

#include <config-X11.h>
#include <wayland-client-core.h>
#if HAVE_X11
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#endif

std::shared_ptr<Klipper> Klipper::self()
{
    static std::weak_ptr<Klipper> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<Klipper> ptr = std::make_shared<Klipper>(nullptr);
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

// config == KGlobal::config for process, otherwise applet
Klipper::Klipper(QObject *parent)
    : QObject(parent)
    , m_clip(SystemClipboard::self())
    , m_historyCycler(new HistoryCycler(this))
    , m_bAutoPaste(false)
    , m_autoPasteInjectionAvailable(false)
    , m_pendingAutoPasteAfterHistorySelection(false)
{
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.klipper"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/klipper"),
                                                 this,
                                                 QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

    m_historyModel = HistoryModel::self();
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &Klipper::onFocusWindowChangedForAutoPaste);
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::slotHistoryChanged);
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::clipboardHistoryUpdated);
    connect(m_historyModel.get(), &HistoryModel::historyMenuEntryActivated, this, &Klipper::slotHistoryMenuEntryActivated);

    // we need that collection, otherwise KToggleAction is not happy :}
    m_collection = new KActionCollection(this);

    m_toggleURLGrabAction = new KToggleAction(this);
    m_collection->addAction(QStringLiteral("clipboard_action"), m_toggleURLGrabAction);
    m_toggleURLGrabAction->setText(i18nc("@action:inmenu Toggle automatic action", "Automatic Action Popup Menu"));
    connect(m_toggleURLGrabAction, &QAction::toggled, this, &Klipper::setURLGrabberEnabled);

    /*
     * Create URL grabber
     */
    m_myURLGrabber = new URLGrabber(this);
    connect(m_myURLGrabber, &URLGrabber::sigPopup, this, &Klipper::showPopupMenu);
    connect(m_historyModel.get(), &HistoryModel::actionInvoked, m_myURLGrabber, &URLGrabber::invokeAction);
    m_historyModel->setURLGrabber(m_myURLGrabber);

#if HAVE_X11
    m_autoPasteInjectionAvailable = KlipperAutoPaste::x11AutoPasteInjectionAvailable();
    if (m_autoPasteInjectionAvailable) {
        Q_EMIT autoPasteSupportChanged(true);
    }
#endif

    /*
     * Load configuration settings
     */
    loadSettings();

    m_clearHistoryAction = m_collection->addAction(QStringLiteral("clear-history"));
    m_clearHistoryAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-clear-history")));
    m_clearHistoryAction->setText(i18nc("@action:inmenu", "C&lear Clipboard History"));
    KGlobalAccel::setGlobalShortcut(m_clearHistoryAction, QKeySequence());
    connect(m_clearHistoryAction, &QAction::triggered, m_historyModel.get(), &HistoryModel::clearHistory);

    m_repeatAction = m_collection->addAction(QStringLiteral("repeat_action"));
    m_repeatAction->setText(i18nc("@action:inmenu", "Manually Invoke Action on Current Clipboard"));
    m_repeatAction->setIcon(QIcon::fromTheme(QStringLiteral("open-menu-symbolic")));
    KGlobalAccel::setGlobalShortcut(m_repeatAction, QKeySequence());
    connect(m_repeatAction, &QAction::triggered, this, &Klipper::slotRepeatAction);

    // add an edit-possibility
    m_editAction = m_collection->addAction(QStringLiteral("edit_clipboard"));
    m_editAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    m_editAction->setText(i18nc("@action:inmenu", "&Edit Contents…"));
    KGlobalAccel::setGlobalShortcut(m_editAction, QKeySequence());
    connect(m_editAction, &QAction::triggered, this, [this] {
        popup()->editCurrentClipboard();
    });

    // add barcode for mobile phones
    m_showBarcodeAction = m_collection->addAction(QStringLiteral("show-barcode"));
    m_showBarcodeAction->setText(i18nc("@action:inmenu", "&Show Barcode…"));
    m_showBarcodeAction->setIcon(QIcon::fromTheme(QStringLiteral("view-barcode-qr")));
    KGlobalAccel::setGlobalShortcut(m_showBarcodeAction, QKeySequence());
    connect(m_showBarcodeAction, &QAction::triggered, this, [this] {
        popup()->showCurrentBarcode();
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
    m_showOnMousePos->setAutoRepeat(false);
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
        QTimer::singleShot(0, this, &Klipper::setupWaylandFakeInputIntegration);
    }
}

void Klipper::setupWaylandFakeInputIntegration()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    auto *connection = KWayland::Client::ConnectionThread::fromApplication(this);
    if (!connection) {
        qCWarning(KLIPPER_LOG) << "KWayland ConnectionThread::fromApplication returned null";
        return;
    }
    auto *registry = new KWayland::Client::Registry(this);
    connect(registry, &KWayland::Client::Registry::fakeInputAnnounced, this, [this, registry](quint32, quint32) {
        tryBindFakeInput(registry);
    });
    connect(registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this, registry](const QByteArray &interface, quint32, quint32) {
        if (interface == QByteArrayLiteral("org_kde_kwin_fake_input")) {
            tryBindFakeInput(registry);
        }
    });
    connect(registry, &KWayland::Client::Registry::interfacesAnnounced, this, [this, registry]() {
        tryBindFakeInput(registry);
    });
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, registry, [registry] {
        delete registry;
    });
    registry->create(connection);
    registry->setup();
    for (int i = 0; i < 3; ++i) {
        connection->roundtrip();
        tryBindFakeInput(registry);
        if (m_fakeInput) {
            break;
        }
    }
    if (!m_fakeInput) {
        qCWarning(KLIPPER_LOG) << "FakeInput not bound after initial roundtrips; retrying";
        const auto retry = [this, connection, registry]() {
            if (m_fakeInput) {
                return;
            }
            connection->roundtrip();
            tryBindFakeInput(registry);
        };
        QTimer::singleShot(100, this, retry);
        QTimer::singleShot(500, this, retry);
        QTimer::singleShot(2000, this, retry);
    }
}

void Klipper::tryBindFakeInput(KWayland::Client::Registry *registry)
{
#if !(defined(__linux__) || defined(__FreeBSD__))
    Q_UNUSED(registry);
    return;
#else
    if (m_fakeInput) {
        return;
    }
    if (!registry->hasInterface(KWayland::Client::Registry::Interface::FakeInput)) {
        return;
    }
    const auto iface = registry->interface(KWayland::Client::Registry::Interface::FakeInput);
    if (iface.name == 0 && iface.version == 0) {
        qCWarning(KLIPPER_LOG) << "Wayland FakeInput interface has invalid name/version";
        return;
    }
    m_fakeInput = registry->createFakeInput(iface.name, iface.version, this);
    if (!m_fakeInput) {
        qCWarning(KLIPPER_LOG) << "FakeInput createFakeInput returned null";
        return;
    }
    if (!m_fakeInput->isValid()) {
        qCWarning(KLIPPER_LOG) << "FakeInput not valid yet; retrying after event loop";
        QTimer::singleShot(0, this, [this]() {
            if (!m_fakeInput || !m_fakeInput->isValid()) {
                qCWarning(KLIPPER_LOG) << "FakeInput still invalid; auto-paste unavailable";
                return;
            }
            m_fakeInput->authenticate(QLatin1String("Klipper"),
                                      i18n("Automatically paste after choosing an entry from the clipboard history."));
            m_autoPasteInjectionAvailable = true;
            Q_EMIT autoPasteSupportChanged(true);
        });
        return;
    }
    m_fakeInput->authenticate(QLatin1String("Klipper"),
                              i18n("Automatically paste after choosing an entry from the clipboard history."));
    m_autoPasteInjectionAvailable = true;
    Q_EMIT autoPasteSupportChanged(true);
#endif
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
    popup()->show();
}

void Klipper::showKlipperManuallyInvokeActionMenu()
{
    slotRepeatAction();
}

void Klipper::reloadConfig()
{
    if (calledFromDBus()) {
        KlipperSettings::self()->sharedConfig()->reparseConfiguration();
        KlipperSettings::self()->read();
    }
    loadSettings();
}

// DBUS - don't call from Klipper itself
void Klipper::setClipboardContents(const QString &s)
{
    if (s.isEmpty())
        return;
    updateTimestamp();
    auto data = std::make_unique<QMimeData>();
    data->setText(s);
    m_clip->setMimeData(data.get(), SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
    m_clip->checkClipData(QClipboard::Clipboard, data.get());
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

void Klipper::saveClipboardHistory()
{
    m_historyModel->saveClipboardHistory();
}

void Klipper::slotStartShowTimer()
{
    m_showTimer.start();
}

void Klipper::loadSettings()
{
    m_bReplayActionInHistory = KlipperSettings::replayActionInHistory();
    m_bAutoPaste = KlipperSettings::autoPaste();

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

KlipperPopup *Klipper::popup()
{
    if (!m_popup) {
        m_popup = std::make_unique<KlipperPopup>();
        connect(m_popup.get(), &KlipperPopup::clipboardPopupOpening, this, &Klipper::clearClipboardPopupAutoPastePending);
    }

    return m_popup.get();
}

void Klipper::showPopupMenu(QMenu *menu)
{
    Q_ASSERT(menu != nullptr);
    menu->hide();
    menu->popup(QCursor::pos());
    QWindow *menuWindow = menu->windowHandle();
    menuWindow->installEventFilter(this);
    if (!menu->windowFlags().testFlag(Qt::Popup)) {
        connect(menuWindow, &QWindow::activeChanged, menu, [menu] {
            if (!menu->windowHandle()->isActive()) {
                menu->hide();
            }
        });
    }
}

bool Klipper::eventFilter(QObject *filtered, QEvent *event)
{
    const bool ret = QObject::eventFilter(filtered, event);
    auto menuWindow = qobject_cast<QWindow *>(filtered);
    if (menuWindow && event->type() == QEvent::Expose && menuWindow->isVisible()) {
        auto integration = PlasmaShellWaylandIntegration::get(menuWindow);
        integration->openUnderCursor();
        integration->setRole(QtWayland::org_kde_plasma_surface::role_appletpopup);

        menuWindow->removeEventFilter(this);
    }
    return ret;
}

// save session on shutdown. Don't simply use the c'tor, as that may not be called.
void Klipper::saveSession()
{
    saveSettings();
}

void Klipper::slotConfigure()
{
    if (KConfigDialog::showDialog(QStringLiteral("preferences"))) {
        // This will never happen, because of the WA_DeleteOnClose below.
        return;
    }

    auto *dlg = new ConfigDialog(nullptr, KlipperSettings::self(), this, m_collection);
    QMetaObject::invokeMethod(dlg,
                              "setHelp",
                              Qt::DirectConnection,
                              Q_ARG(QString, QString::fromLatin1("preferences")),
                              Q_ARG(QString, QString::fromLatin1("org.kde.klipper")));
    // This is necessary to ensure that the dialog is recreated
    // and therefore the controls are initialised from the current
    // Klipper settings every time that it is shown.
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &KConfigDialog::settingsChanged, this, &Klipper::reloadConfig);
    dlg->show();
}

void Klipper::slotPopupMenu()
{
    if (popup()->isVisible()) {
        popup()->hide();
    } else {
        popup()->show();
    }
}

void Klipper::slotRepeatAction()
{
    auto top = std::static_pointer_cast<const HistoryItem>(m_historyModel->first());
    if (top) {
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
    if (!isTop) {
        return;
    }

    QString &lastURLGrabberText = m_clip->isLocked(QClipboard::Selection) ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    if (auto item = m_historyModel->first(); m_bURLGrabber && item && item->type() == HistoryItemType::Text) {
        m_myURLGrabber->checkNewData(std::const_pointer_cast<const HistoryItem>(m_historyModel->first()));

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

    if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    if (m_bReplayActionInHistory && m_bURLGrabber) {
        slotRepeatAction();
    }
}

bool Klipper::isAutoPasteSupported() const
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        return m_autoPasteInjectionAvailable;
    }
#endif
#if defined(__linux__) || defined(__FreeBSD__)
    if (KWindowSystem::isPlatformWayland()) {
        return m_fakeInput && m_fakeInput->isValid();
    }
#endif
    return false;
}

bool Klipper::isAutoPasteInjectReady() const
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        return m_autoPasteInjectionAvailable;
    }
#endif
#if defined(__linux__) || defined(__FreeBSD__)
    if (KWindowSystem::isPlatformWayland()) {
        return m_fakeInput && m_fakeInput->isValid() && m_autoPasteInjectionAvailable;
    }
#endif
    return false;
}

void Klipper::slotHistoryMenuEntryActivated()
{
    if (!m_bAutoPaste) {
        return;
    }
    if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    m_pendingAutoPasteAfterHistorySelection = true;
}

void Klipper::clearClipboardPopupAutoPastePending()
{
    m_pendingAutoPasteAfterHistorySelection = false;
}

void Klipper::onFocusWindowChangedForAutoPaste(QWindow *focus)
{
    if (!m_pendingAutoPasteAfterHistorySelection || !m_bAutoPaste) {
        return;
    }
    // Wait until focus leaves the clipboard popup (still visible: selection not finished hiding).
    if (m_popup && focus == m_popup.get() && m_popup->isVisible()) {
        return;
    }
    m_pendingAutoPasteAfterHistorySelection = false;
    QTimer::singleShot(0, this, [this]() {
        if (!m_bAutoPaste) {
            return;
        }
        if (m_clip->isLocked(QClipboard::Selection) || m_clip->isLocked(QClipboard::Clipboard)) {
            return;
        }
        simulatePaste();
    });
}

void Klipper::simulatePaste()
{
    if (!isAutoPasteInjectReady()) {
        return;
    }
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        simulatePasteX11();
        return;
    }
#endif
#if defined(__linux__) || defined(__FreeBSD__)
    if (m_fakeInput && m_fakeInput->isValid()) {
        KlipperAutoPaste::injectPasteShortcutWaylandFakeInput(m_fakeInput, KlipperAutoPaste::pasteChordFromStandard());
        return;
    }
    qCWarning(KLIPPER_LOG) << "Auto-paste: KWin FakeInput is unavailable";
#endif
}

#if HAVE_X11
void Klipper::simulatePasteX11()
{
    auto *x11 = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!x11) {
        qCWarning(KLIPPER_LOG) << "Auto-paste: no X11 application interface";
        return;
    }
    KlipperAutoPaste::injectPasteShortcutX11(static_cast<void *>(x11->display()));
}
#endif

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

void Klipper::updateTimestamp()
{
#if HAVE_X11
    if (auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        xcb_aux_sync(interface->connection());
    }
#endif
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

#include "moc_klipper.cpp"
