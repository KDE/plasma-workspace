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

#include "configdialog.h"
#include "historycycler.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipperpopup.h"
#include "klippersettings.h"
#include "systemclipboard.h"

#include <Prison/Barcode>

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
    , m_quitAction(nullptr)
    , m_plasmashell(nullptr)
{
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.klipper"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/klipper"),
                                                 this,
                                                 QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

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
    connect(m_editAction, &QAction::triggered, m_popup.get(), &KlipperPopup::editCurrentClipboard);

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
    // NOTE: not used atm - kregexpeditor is not ported to kde4
    m_bUseGUIRegExpEditor = KlipperSettings::useGUIRegExpEditor();

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
    saveSettings();
}

void Klipper::slotConfigure()
{
    if (KConfigDialog::showDialog(QStringLiteral("preferences"))) {
        // This will never happen, because of the WA_DeleteOnClose below.
        return;
    }

    auto *dlg = new ConfigDialog(nullptr, KlipperSettings::self(), this, m_collection);
    QMetaObject::invokeMethod(dlg, "setHelp", Qt::DirectConnection, Q_ARG(QString, QString::fromLatin1("preferences")));
    // This is necessary to ensure that the dialog is recreated
    // and therefore the controls are initialised from the current
    // Klipper settings every time that it is shown.
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    connect(dlg, &KConfigDialog::settingsChanged, this, &Klipper::reloadConfig);
    dlg->show();
}

void Klipper::slotPopupMenu()
{
    m_popup->show();
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
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, dlg);
    buttons->button(QDialogButtonBox::Ok)->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttons, &QDialogButtonBox::accepted, dlg.data(), &QDialog::accept);
    connect(dlg.data(), &QDialog::finished, dlg.data(), &QDialog::deleteLater);

    auto *mw = new QWidget(dlg);
    auto *layout = new QHBoxLayout(mw);

    {
        auto qrCode = Prison::Barcode::create(Prison::QRCode);
        if (qrCode) {
            if (item) {
                qrCode->setData(item->text());
            }
            auto *qrCodeLabel = new BarcodeLabel(std::move(*qrCode), mw);
            layout->addWidget(qrCodeLabel);
        }
    }
    {
        auto dataMatrix = Prison::Barcode::create(Prison::DataMatrix);
        if (dataMatrix) {
            if (item) {
                dataMatrix->setData(item->text());
            }
            auto *dataMatrixLabel = new BarcodeLabel(std::move(*dataMatrix), mw);
            layout->addWidget(dataMatrixLabel);
        }
    }

    mw->setFocus();
    auto *vBox = new QVBoxLayout(dlg);
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

#include "moc_klipper.cpp"
