/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QClipboard>
#include <QObject>
#include <QTimer>

#include <KSystemClipboard>

#include "klipper_export.h"

class KSystemClipboard;
class HistoryItem;

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
    Ignore(int &locklevel)
        : lockLevelRef(locklevel)
    {
        lockLevelRef++;
    }
    ~Ignore()
    {
        lockLevelRef--;
    }
    Ignore(Ignore &&other)
        : lockLevelRef(other.lockLevelRef)
    {
    }

private:
    int &lockLevelRef;
};

/**
 * This class filters out invalid data from KSystemClipboard, and only emits newClipData when the data is valid.
 */
class KLIPPER_EXPORT SystemClipboard : public QObject
{
    Q_OBJECT

public:
    /**
     * The selection modes
     *
     * Don't use 1, as I use that as a guard against passing
     * a boolean true as a mode.
     */
    enum SelectionMode {
        Clipboard = 2,
        Selection = 4,
    };

    enum class ClipboardUpdateReason {
        UpdateClipboard,
        SyncSelection,
        PreventEmptyClipboard,
    };

    static std::shared_ptr<SystemClipboard> self();
    ~SystemClipboard() override;

    void clear(QClipboard::Mode mode);
    void setMimeData(const std::shared_ptr<const HistoryItem> &data, int mode, ClipboardUpdateReason updateReason = ClipboardUpdateReason::UpdateClipboard);

    bool isLocked(QClipboard::Mode mode);

Q_SIGNALS:
    // Only emitted when data is valid
    void newClipData(QClipboard::Mode mode, const QMimeData *data);
    void ignored(QClipboard::Mode mode);
    void receivedEmptyClipboard(QClipboard::Mode mode);

private Q_SLOTS:
    /**
     * Check data in clipboard, and if it passes these checks,
     * emit newClipData
     */
    void checkClipData(QClipboard::Mode mode);

    void slotClearOverflow();
    void slotCheckPending();

private:
    explicit SystemClipboard();

    bool blockFetchingNewData();

    KSystemClipboard *m_clip = nullptr;

    /**
     * Avoid reacting to our own changes, using this
     * lock.
     * Don't manupulate this object directly... use the Ignore struct
     * instead
     */
    int m_selectionLocklevel = 0;
    int m_clipboardLocklevel = 0;
    int m_overflowCounter = 0;
    QTimer m_overflowClearTimer;
    QTimer m_pendingCheckTimer;
    bool m_pendingContentsCheck = false;
};
