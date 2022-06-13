/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "klipperpopup.h"

#include "klipper_debug.h"
#include <QApplication>
#include <QKeyEvent>
#include <QScreen>
#include <QWidgetAction>

#include <KHelpMenu>
#include <KLineEdit>
#include <KLocalizedString>
#include <KWindowInfo>

#include "history.h"
#include "klipper.h"
#include "popupproxy.h"

namespace
{
static const int TOP_HISTORY_ITEM_INDEX = 2;
}

// #define DEBUG_EVENTS__

#ifdef DEBUG_EVENTS__
kdbgstream &operator<<(kdbgstream &stream, const QKeyEvent &e)
{
    stream << "(QKeyEvent(text=" << e.text() << ",key=" << e.key() << (e.isAccepted() ? ",accepted" : ",ignored)") << ",count=" << e.count();
    if (e.modifiers() & Qt::AltModifier) {
        stream << ",ALT";
    }
    if (e.modifiers() & Qt::ControlModifier) {
        stream << ",CTRL";
    }
    if (e.modifiers() & Qt::MetaModifier) {
        stream << ",META";
    }
    if (e.modifiers() & Qt::ShiftModifier) {
        stream << ",SHIFT";
    }
    if (e.isAutoRepeat()) {
        stream << ",AUTOREPEAT";
    }
    stream << ")";

    return stream;
}
#endif

KlipperPopup::KlipperPopup(History *history)
    : m_dirty(true)
    , m_textForEmptyHistory(i18n("Clipboard is empty"))
    , m_textForNoMatch(i18n("No matches"))
    , m_history(history)
    , m_helpMenu(nullptr)
    , m_popupProxy(nullptr)
    , m_filterWidget(nullptr)
    , m_filterWidgetAction(nullptr)
    , m_nHistoryItems(0)
    , m_showHelp(true)
    , m_lastEvent(nullptr)
{
    ensurePolished();
    KWindowInfo windowInfo(winId(), NET::WMGeometry);
    QRect geometry = windowInfo.geometry();
    QScreen *screen = QGuiApplication::screenAt(geometry.center());
    if (screen == nullptr) {
        screen = QGuiApplication::screens()[0];
    }
    int menuHeight = (screen->geometry().height()) * 3 / 4;
    int menuWidth = (screen->geometry().width()) * 1 / 3;

    m_popupProxy = new PopupProxy(this, menuHeight, menuWidth);

    connect(this, &KlipperPopup::aboutToShow, this, &KlipperPopup::slotAboutToShow);
}

KlipperPopup::~KlipperPopup()
{
}

void KlipperPopup::slotAboutToShow()
{
    if (m_filterWidget) {
        if (!m_filterWidget->text().isEmpty()) {
            m_dirty = true;
            m_filterWidget->clear();
        }
    }
    ensureClean();
}

void KlipperPopup::ensureClean()
{
    // If the history is unchanged since last menu build, the is no reason
    // to rebuild it,
    if (m_dirty) {
        rebuild();
    }
}

void KlipperPopup::buildFromScratch()
{
    addSection(QIcon::fromTheme(QStringLiteral("klipper")), i18n("Klipper - Clipboard Tool"));

    m_filterWidget = new KLineEdit(this);
    m_filterWidget->setFocusPolicy(Qt::NoFocus);
    m_filterWidget->setPlaceholderText(i18n("Search…"));
    m_filterWidgetAction = new QWidgetAction(this);
    m_filterWidgetAction->setDefaultWidget(m_filterWidget);
    addAction(m_filterWidgetAction);

    addSeparator();
    for (int i = 0; i < m_actions.count(); i++) {
        if (i + 1 == m_actions.count() && m_showHelp) {
            if (!m_helpMenu) {
                m_helpMenu = new KHelpMenu(this, i18n("KDE cut & paste history utility"), false);
            }
            addMenu(m_helpMenu->menu())->setIcon(QIcon::fromTheme(QStringLiteral("help-contents")));
            addSeparator();
        }

        addAction(m_actions.at(i));
    }
}

void KlipperPopup::rebuild(const QString &filter)
{
    if (actions().isEmpty()) {
        buildFromScratch();
    } else {
        for (int i = 0; i < m_nHistoryItems; i++) {
            Q_ASSERT(TOP_HISTORY_ITEM_INDEX < actions().count());

            // The old actions allocated by KlipperPopup::rebuild()
            // and PopupProxy::tryInsertItem() are deleted here when
            // the menu is rebuilt.
            QAction *action = actions().at(TOP_HISTORY_ITEM_INDEX);
            removeAction(action);
            action->deleteLater();
        }
    }

    // We search case insensitive until one uppercased character appears in the search term
    QRegularExpression filterexp(filter, filter.toLower() == filter ? QRegularExpression::CaseInsensitiveOption : QRegularExpression::NoPatternOption);

    QPalette palette = m_filterWidget->palette();
    if (filterexp.isValid()) {
        palette.setColor(m_filterWidget->foregroundRole(), palette.color(foregroundRole()));
    } else {
        palette.setColor(m_filterWidget->foregroundRole(), Qt::red);
    }
    m_nHistoryItems = m_popupProxy->buildParent(TOP_HISTORY_ITEM_INDEX, filterexp);
    if (m_nHistoryItems == 0) {
        if (m_history->empty()) {
            insertAction(actions().at(TOP_HISTORY_ITEM_INDEX), new QAction(m_textForEmptyHistory, this));
        } else {
            palette.setColor(m_filterWidget->foregroundRole(), Qt::red);
            insertAction(actions().at(TOP_HISTORY_ITEM_INDEX), new QAction(m_textForNoMatch, this));
        }
        m_nHistoryItems++;
    } else {
        if (history()->topIsUserSelected()) {
            actions().at(TOP_HISTORY_ITEM_INDEX)->setCheckable(true);
            actions().at(TOP_HISTORY_ITEM_INDEX)->setChecked(true);
        }
    }
    m_filterWidget->setPalette(palette);
    m_dirty = false;
}

void KlipperPopup::slotTopIsUserSelectedSet()
{
    if (!m_dirty && m_nHistoryItems > 0 && history()->topIsUserSelected()) {
        actions().at(TOP_HISTORY_ITEM_INDEX)->setCheckable(true);
        actions().at(TOP_HISTORY_ITEM_INDEX)->setChecked(true);
    }
}

void KlipperPopup::plugAction(QAction *action)
{
    m_actions.append(action);
}

/* virtual */
void KlipperPopup::keyPressEvent(QKeyEvent *e)
{
    // Most events are send down directly to the m_filterWidget.
    // If the m_filterWidget does not handle the event, it will
    // come back to this method. Remembering the last event stops
    // the infinite event loop
    if (m_lastEvent == e) {
        m_lastEvent = nullptr;
        return;
    }
    m_lastEvent = e;
    // If alt-something is pressed, select a shortcut
    // from the menu. Do this by sending a keyPress
    // without the alt-modifier to the superobject.
    if (e->modifiers() & Qt::AltModifier) {
        QKeyEvent ke(QEvent::KeyPress, e->key(), e->modifiers() ^ Qt::AltModifier, e->text(), e->isAutoRepeat(), e->count());
        QMenu::keyPressEvent(&ke);
#ifdef DEBUG_EVENTS__
        qCDebug(KLIPPER_LOG) << "Passing this event to ancestor (KMenu): " << e << "->" << ke;
#endif
        if (ke.isAccepted()) {
            e->accept();
            return;
        } else {
            e->ignore();
        }
    }

    // Otherwise, send most events to the search
    // widget, except a few used for navigation:
    // These go to the superobject.
    switch (e->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Right:
    case Qt::Key_Left:
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Escape: {
#ifdef DEBUG_EVENTS__
        qCDebug(KLIPPER_LOG) << "Passing this event to ancestor (KMenu): " << e;
#endif
        QMenu::keyPressEvent(e);

        break;
    }
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        QMenu::keyPressEvent(e);
        this->hide();

        if (activeAction() == m_filterWidgetAction)
            setActiveAction(actions().at(TOP_HISTORY_ITEM_INDEX));

        break;
    }
    default: {
#ifdef DEBUG_EVENTS__
        qCDebug(KLIPPER_LOG) << "Passing this event down to child (KLineEdit): " << e;
#endif
        setActiveAction(actions().at(actions().indexOf(m_filterWidgetAction)));
        QString lastString = m_filterWidget->text();

        QApplication::sendEvent(m_filterWidget, e);
        if (m_filterWidget->text() != lastString) {
            m_dirty = true;
            rebuild(m_filterWidget->text());
        }

        break;
    } // default:
    } // case
    m_lastEvent = nullptr;
}

void KlipperPopup::slotSetTopActive()
{
    if (actions().size() > TOP_HISTORY_ITEM_INDEX) {
        setActiveAction(actions().at(TOP_HISTORY_ITEM_INDEX));
    }
}
