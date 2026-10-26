/*

    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipper.h"

#include "klipper_debug.h"
#include <QDBusConnection>
#include <QMimeData>

#include <KActionCollection>
#include <KGlobalAccel>
#include <KLocalizedString>
#include <KNotification>
#include <KToggleAction>

#include "configdialog.h"
#include "historycycler.h"
#include "historyitem.h"
#include "historymodel.h"
#include "klipperpopup.h"
#include "klippersettings.h"
#include "systemclipboard.h"
#include "urlgrabber.h"

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
{
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.klipper"));
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/klipper"),
                                                 this,
                                                 QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);

    m_historyModel = HistoryModel::self();
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::clipboardHistoryUpdated);
    connect(m_historyModel.get(), &HistoryModel::changed, this, &Klipper::slotHistoryChanged);

    // Prevent the action menu from popping up on start, the top history entry is put on the
    // clipboard when Klipper starts, and that arrives as a new clipboard event.
    if (const auto top = m_historyModel->first()) {
        m_lastURLGrabberTextSelection = top->text();
        m_lastURLGrabberTextClipboard = top->text();
    }

    // Forget the last text when the grabber is (re-)enabled, so that the menu is offered
    // again for the current clipboard contents.
    connect(KlipperSettings::self(), &KlipperSettings::URLGrabberEnabledChanged, this, [this] {
        m_lastURLGrabberTextSelection.clear();
        m_lastURLGrabberTextClipboard.clear();
    });

    // we need that collection, otherwise KToggleAction is not happy :}
    m_collection = new KActionCollection(this);
    m_collection->setComponentName(QStringLiteral("org.kde.klipper"));
    m_collection->setComponentDisplayName(i18nc("@title shortcut group", "Clipboard"));

    m_toggleURLGrabAction = new KToggleAction(this);
    m_collection->addAction(QStringLiteral("clipboard_action"), m_toggleURLGrabAction);
    m_toggleURLGrabAction->setText(i18nc("@action:inmenu Toggle automatic action", "Toggle Automatic Popping Up of Action Menu"));
    m_toggleURLGrabAction->setIcon(QIcon::fromTheme(QStringLiteral("show-menu")));
    KGlobalAccel::setGlobalShortcut(m_toggleURLGrabAction, QKeySequence());
    connect(m_toggleURLGrabAction, &QAction::toggled, this, &Klipper::setURLGrabberEnabled);

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
    connect(m_repeatAction, &QAction::triggered, this, [this] {
        showActionMenu(0);
    });

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
    m_cycleNextAction->setText(i18nc("@action:inmenu", "Rotate History Down"));
    m_cycleNextAction->setIcon(QIcon::fromTheme(QStringLiteral("go-down")));
    KGlobalAccel::setGlobalShortcut(m_cycleNextAction, QKeySequence());
    connect(m_cycleNextAction, &QAction::triggered, this, &Klipper::slotCycleNext);
    m_cyclePrevAction = m_collection->addAction(QStringLiteral("cyclePrevAction"));
    m_cyclePrevAction->setText(i18nc("@action:inmenu", "Rotate History Up"));
    m_cyclePrevAction->setIcon(QIcon::fromTheme(QStringLiteral("go-up")));
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
}

Klipper::~Klipper()
{
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
    showActionMenu(0);
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
    auto data = std::make_unique<QMimeData>();
    data->setText(s);
    m_clip->setMimeData(data.get(), SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
    // Without this, klipper gets confused whether it holds passwords
    // because the write above is under lock and does not emit changed signal
    m_clip->checkClipData(QClipboard::Clipboard, data.get());
    m_clip->checkClipData(QClipboard::Selection, data.get());
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardContents()
{
    m_historyModel->clearBuffers();
}

// DBUS - don't call from Klipper itself
void Klipper::clearClipboardHistory()
{
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

void Klipper::slotHistoryChanged(bool isTop)
{
    if (!isTop) {
        return;
    }

    const bool urlGrabberEnabled = KlipperSettings::uRLGrabberEnabled();
    const bool selectionLocked = m_clip->isLocked(QClipboard::Selection);
    QString &lastURLGrabberText = selectionLocked ? m_lastURLGrabberTextSelection : m_lastURLGrabberTextClipboard;
    const auto item = m_historyModel->first();
    const ActionList actions = urlGrabberEnabled ? URLGrabber::loadActions() : ActionList();
    if (urlGrabberEnabled && item && item->allTypes().testFlag(HistoryItemType::Text)) {
        // Do not offer the menu for the same action twice (ie. you select and copy, or press ctrl+C twice)
        if (item->text() != lastURLGrabberText) {
            lastURLGrabberText = item->text();
            if (!URLGrabber::matchingActions(item->text(), true, actions).isEmpty()) {
                popup()->showActionMenu(0, true);
            }
        }
    } else {
        lastURLGrabberText.clear();
    }

    if (selectionLocked || m_clip->isLocked(QClipboard::Clipboard)) {
        return;
    }
    // Selecting an item from the history replays all applicable actions, not only the automatic ones
    if (urlGrabberEnabled && KlipperSettings::replayActionInHistory() && item && !URLGrabber::matchingActions(item->text(), false, actions).isEmpty()) {
        popup()->showActionMenu(0, false);
    }
}

void Klipper::loadSettings()
{
    m_historyModel->loadSettings();
}

void Klipper::showActionMenu(int itemIndex)
{
    popup()->showActionMenu(itemIndex);
}

void Klipper::saveSettings()
{
    KlipperSettings::setVersion(QStringLiteral(KLIPPER_VERSION_STRING));
    KlipperSettings::self()->save();

    // other settings should be saved automatically by KConfigDialog
}

KlipperPopup *Klipper::popup()
{
    if (!m_popup) {
        m_popup = std::make_unique<KlipperPopup>();
    }

    return m_popup.get();
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

void Klipper::setURLGrabberEnabled(bool enable)
{
    if (enable != KlipperSettings::uRLGrabberEnabled()) {
        KlipperSettings::setURLGrabberEnabled(enable);
    }

    m_toggleURLGrabAction->setChecked(enable);
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
        result += i18n("Newer: ");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemPrev->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("<tr><td>");
    result += i18n("Current: ");
    result += QLatin1String("</td><td><b>");
    result += font_metrics.elidedText(item->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
    result += QLatin1String("</b></td></tr>");

    if (itemNext) {
        result += QLatin1String("<tr><td>");
        result += i18n("Older: ");
        result += QLatin1String("</td><td>");
        result += font_metrics.elidedText(itemNext->text().simplified().toHtmlEscaped(), Qt::ElideMiddle, WIDTH_IN_PIXEL);
        result += QLatin1String("</td></tr>");
    }

    result += QLatin1String("</table>");
    return result;
}

#include "moc_klipper.cpp"
