/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemclipboard.h"

#include <chrono>

#include <QApplication>
#include <QMimeData>
#include <QWidget>

#include <KWindowSystem>

#include "../c_ptr.h"
#include "config-X11.h"
#include "historyitem.h"
#include "klipper_debug.h"
#include <wayland-client-core.h>

#if HAVE_X11
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/xcb_event.h>
#endif // HAVE_X11

using namespace std::chrono_literals;

namespace
{
// Protection against too many clipboard data changes. Lyx responds to clipboard data
// requests with setting new clipboard data, so if Lyx takes over clipboard,
// Klipper notices, requests this data, this triggers "new" clipboard contents
// from Lyx, so Klipper notices again, requests this data, ... you get the idea.
constexpr auto MAX_CLIPBOARD_CHANGES = 10; // max changes per second

bool ignoreClipboardChanges()
{
    // Changing a spinbox in klipper's config-dialog causes the lineedit-contents
    // of the spinbox to be selected and hence the clipboard changes. But we don't
    // want all those items in klipper's history. See #41917
    const auto app = qobject_cast<QApplication *>(QCoreApplication::instance());
    if (!app) {
        return false;
    }

    QWidget *focusWidget = app->focusWidget();
    if (focusWidget) {
        if (focusWidget->inherits("QSpinBox")
            || (focusWidget->parentWidget() && focusWidget->inherits("QLineEdit") && focusWidget->parentWidget()->inherits("QSpinWidget"))) {
            return true;
        }
    }

    return false;
}

void x11RoundTrip()
{
#if HAVE_X11
    if (auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()) {
        const auto cookie = xcb_get_input_focus(interface->connection());
        xcb_generic_error_t *error = nullptr;
        UniqueCPointer<xcb_get_input_focus_reply_t> sync(xcb_get_input_focus_reply(interface->connection(), cookie, &error));
        if (error) {
            free(error);
        }
    }
#endif
}
}

std::shared_ptr<SystemClipboard> SystemClipboard::self()
{
    static std::weak_ptr<SystemClipboard> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<SystemClipboard> ptr{new SystemClipboard};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

SystemClipboard::SystemClipboard()
    : QObject(nullptr)
    , m_clip(KSystemClipboard::instance())
{
    x11RoundTrip(); // read initial X user time
    connect(m_clip, &KSystemClipboard::changed, this, &SystemClipboard::checkClipData);

    m_pendingCheckTimer.setSingleShot(true);
    connect(&m_pendingCheckTimer, &QTimer::timeout, this, &SystemClipboard::slotCheckPending);
    connect(&m_overflowClearTimer, &QTimer::timeout, this, &SystemClipboard::slotClearOverflow);
}

SystemClipboard::~SystemClipboard()
{
}

void SystemClipboard::clear(SelectionMode mode)
{
    Q_ASSERT((mode & 1) == 0); // Warn if trying to pass a boolean as a mode.

    if (mode & Selection) {
        Ignore lock(m_selectionLocklevel);
        m_clip->clear(QClipboard::Selection);
    }
    if (mode & Clipboard) {
        Ignore lock(m_clipboardLocklevel);
        m_clip->clear(QClipboard::Clipboard);
    }
}

void SystemClipboard::setMimeData(const HistoryItemConstPtr &data, SelectionMode mode, ClipboardUpdateReason updateReason)
{
    Q_ASSERT((mode & 1) == 0); // Warn if trying to pass a boolean as a mode.

    if (mode & Selection) {
        Ignore lock(m_selectionLocklevel);
        QMimeData *mimeData = data->mimeData();
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            mimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        }
        qCDebug(KLIPPER_LOG) << "Setting selection to <" << mimeData->text() << ">";
        m_clip->setMimeData(mimeData, QClipboard::Selection);
    }
    if (mode & Clipboard) {
        QMimeData *mimeData = data->mimeData(); // NOTE: this has to be a new mimeData because setMimeData will take ownership
        if (updateReason == ClipboardUpdateReason::PreventEmptyClipboard) {
            mimeData->setData(QStringLiteral("application/x-kde-onlyReplaceEmpty"), "1");
        } else if (updateReason == ClipboardUpdateReason::SyncSelection) {
            // When plasmashell is not focused, klipper will not receive new clip data immediately. This type is used to filter out selections.
            mimeData->setData(QStringLiteral("application/x-kde-syncselection"), "1");
        }
        qCDebug(KLIPPER_LOG) << "Setting clipboard to <" << mimeData->text() << ">";
        QMetaObject::invokeMethod(
            this,
            [this, mimeData]() {
                Ignore lock(m_clipboardLocklevel);
                m_clip->setMimeData(mimeData, QClipboard::Clipboard);
            },
            updateReason == ClipboardUpdateReason::SyncSelection ? Qt::QueuedConnection : Qt::DirectConnection);
    }
}

bool SystemClipboard::isLocked(QClipboard::Mode mode)
{
    return mode == QClipboard::Selection ? m_selectionLocklevel : m_clipboardLocklevel;
}

void SystemClipboard::checkClipData(QClipboard::Mode mode)
{
    if ((mode == QClipboard::Clipboard && m_clipboardLocklevel) || (mode == QClipboard::Selection && m_selectionLocklevel)) {
        return;
    }

    if (mode == QClipboard::Selection && blockFetchingNewData()) {
        return;
    }

    Ignore lock(mode == QClipboard::Selection ? m_selectionLocklevel : m_clipboardLocklevel);

    // internal to klipper, ignoring QSpinBox selections
    if (ignoreClipboardChanges()) {
        // keep our old clipboard, thanks
        // This won't quite work, but it's close enough for now.
        // The trouble is that the top selection =! top clipboard
        // but we don't track that yet. We will....
        Q_EMIT ignored(mode);
        return;
    }

    qCDebug(KLIPPER_LOG) << "Checking clip data";

    const QMimeData *data = m_clip->mimeData(mode);

    if (!data) {
        Q_EMIT receivedEmptyClipboard(mode);
        return;
    } else if (data->formats().isEmpty()) {
        // Might be a timeout. Try again
        x11RoundTrip();
        data = m_clip->mimeData(mode);
        if (!data || data->formats().isEmpty()) {
            qCDebug(KLIPPER_LOG) << "was empty. Retried, now still empty";
            Q_EMIT receivedEmptyClipboard(mode);
            return;
        }
    }

    if (!data->hasUrls() && !data->hasText() && !data->hasImage()) {
        return; // unknown, ignore
    }

    if (data->hasFormat(QStringLiteral("application/x-kde-syncselection"))) {
        return;
    }

    Q_ASSERT_X((mode == QClipboard::Clipboard && m_clipboardLocklevel == 1) || (mode == QClipboard::Selection && m_selectionLocklevel == 1),
               Q_FUNC_INFO,
               qPrintable(QStringLiteral("%1 %2").arg(QString::number(m_clipboardLocklevel), QString::number(m_selectionLocklevel))));
    Q_EMIT newClipData(mode, data);
}

void SystemClipboard::slotClearOverflow()
{
    m_overflowClearTimer.stop();

    if (m_overflowCounter > MAX_CLIPBOARD_CHANGES) {
        qCDebug(KLIPPER_LOG) << "App owning the clipboard/selection is lame";
        // update to the latest data - this unfortunately may trigger the problem again
        if (!m_selectionLocklevel && blockFetchingNewData()) {
            checkClipData(QClipboard::Selection); // Always selection
        }
    }

    m_overflowCounter = 0;
}

void SystemClipboard::slotCheckPending()
{
    if (!m_pendingContentsCheck) {
        return;
    }
    m_pendingContentsCheck = false; // blockFetchingNewData() will be called again
    x11RoundTrip();
    checkClipData(QClipboard::Selection); // Always selection
}

bool SystemClipboard::blockFetchingNewData()
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
    auto interface = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    if (!interface) {
        return false;
    }
    xcb_connection_t *c = interface->connection();
    const xcb_query_pointer_cookie_t cookie = xcb_query_pointer_unchecked(c, DefaultRootWindow(interface->display()));
    UniqueCPointer<xcb_query_pointer_reply_t> queryPointer(xcb_query_pointer_reply(c, cookie, nullptr));
    if (!queryPointer) {
        return false;
    }
    if (((queryPointer->mask & (XCB_KEY_BUT_MASK_SHIFT | XCB_KEY_BUT_MASK_BUTTON_1)) == XCB_KEY_BUT_MASK_SHIFT) // BUG: 85198
        || ((queryPointer->mask & XCB_KEY_BUT_MASK_BUTTON_1) == XCB_KEY_BUT_MASK_BUTTON_1)) { // BUG: 80302
        m_pendingContentsCheck = true;
        m_pendingCheckTimer.start(100ms);
        return true;
    }
    m_pendingContentsCheck = false;
    if (m_overflowCounter == 0) {
        m_overflowClearTimer.start(1s);
    }

    if (++m_overflowCounter > MAX_CLIPBOARD_CHANGES) {
        return true;
    }
#endif
    return false;
}

#include "moc_systemclipboard.cpp"