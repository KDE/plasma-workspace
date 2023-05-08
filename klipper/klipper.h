/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "config-klipper.h"

#include <QClipboard>
#include <QElapsedTimer>
#include <QPointer>
#include <QTimer>

#include <KTextEdit>

#include "urlgrabber.h"

class KToggleAction;
class KActionCollection;
class KlipperPopup;
class URLGrabber;
class QTime;
class History;
class QAction;
class QMenu;
class QMimeData;
class HistoryItem;
class KNotification;
class KSystemClipboard;

namespace KWayland
{
namespace Client
{
class PlasmaShell;
}
}

enum class KlipperMode {
    Standalone,
    DataEngine,
};

class ClipboardContentTextEdit : public KTextEdit
{
    Q_OBJECT
public:
    ClipboardContentTextEdit(QWidget *parent);
    void keyPressEvent(QKeyEvent *event) override;
    Q_SIGNAL void done();
};

class Klipper : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.klipper.klipper")

public Q_SLOTS:
    Q_SCRIPTABLE QString getClipboardContents();
    Q_SCRIPTABLE void setClipboardContents(const QString &s);
    Q_SCRIPTABLE void clearClipboardContents();
    Q_SCRIPTABLE void clearClipboardHistory();
    Q_SCRIPTABLE void saveClipboardHistory();
    Q_SCRIPTABLE QStringList getClipboardHistoryMenu();
    Q_SCRIPTABLE QString getClipboardHistoryItem(int i);
    Q_SCRIPTABLE void showKlipperPopupMenu();
    Q_SCRIPTABLE void showKlipperManuallyInvokeActionMenu();

public:
    Klipper(QObject *parent, const KSharedConfigPtr &config, KlipperMode mode = KlipperMode::Standalone);
    ~Klipper() override;

    bool eventFilter(QObject *object, QEvent *event) override;

    /**
     * Get clipboard history (the "document")
     */
    History *history()
    {
        return m_history;
    }

    URLGrabber *urlGrabber() const
    {
        return m_myURLGrabber;
    }

    void saveSettings() const;

    QMenu *actionsPopup() const
    {
        return m_actionsPopup;
    }

    KlipperPopup *popup()
    {
        return m_popup;
    }

    void editData(const QSharedPointer<const HistoryItem> &item);
    void showBarcode(const QSharedPointer<const HistoryItem> &item);

public Q_SLOTS:
    void saveSession();
    void slotHistoryTopChanged();
    void slotConfigure();
    void slotCycleNext();
    void slotCyclePrev();

protected:
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
        PreventEmptyClipboard,
    };

    /**
     * Loads history from disk.
     */
    bool loadHistory();

    /**
     * Save history to disk
     * @param empty save empty history instead of actual history
     */
    void saveHistory(bool empty = false);

    /**
     * Check data in clipboard, and if it passes these checks,
     * store the data in the clipboard history.
     */
    void checkClipData(bool selectionMode);

    /**
     * Enter clipboard data in the history.
     */
    QSharedPointer<HistoryItem> applyClipChanges(const QMimeData *data, bool selectionMode);

    void setClipboard(const HistoryItem &item, int mode, ClipboardUpdateReason updateReason = ClipboardUpdateReason::UpdateClipboard);
    bool ignoreClipboardChanges() const;

    KSharedConfigPtr config() const
    {
        return m_config;
    }

Q_SIGNALS:
    void passivePopup(const QString &caption, const QString &text);
    void editFinished(QSharedPointer<const HistoryItem> item, int result);

public Q_SLOTS:
    void slotPopupMenu();
    void slotAskClearHistory();
    void setURLGrabberEnabled(bool);

protected Q_SLOTS:
    void showPopupMenu(QMenu *);
    void slotRepeatAction();
    void disableURLGrabber();

private Q_SLOTS:
    void newClipData(QClipboard::Mode);
    void slotClearClipboard();

    void slotHistoryChanged();

    void slotQuit();
    void slotStartShowTimer();

    void slotClearOverflow();
    void slotCheckPending();

    void loadSettings();

private:
    static void updateTimestamp();

    KSystemClipboard *m_clip;

    QElapsedTimer m_showTimer;

    History *m_history;
    KlipperPopup *m_popup;
    int m_overflowCounter;

    KToggleAction *m_toggleURLGrabAction;
    QAction *m_clearHistoryAction;
    QAction *m_repeatAction;
    QAction *m_editAction;
    QAction *m_showBarcodeAction;
    QAction *m_configureAction;
    QAction *m_quitAction;
    QAction *m_cycleNextAction;
    QAction *m_cyclePrevAction;
    QAction *m_showOnMousePos;

    bool m_bKeepContents : 1;
    bool m_bURLGrabber : 1;
    bool m_bReplayActionInHistory : 1;
    bool m_bUseGUIRegExpEditor : 1;
    bool m_bNoNullClipboard : 1;
    bool m_bIgnoreSelection : 1;
    bool m_bSynchronize : 1;
    bool m_bSelectionTextOnly : 1;
    bool m_bIgnoreImages : 1;

    /**
     * Avoid reacting to our own changes, using this
     * lock.
     * Don't manupulate this object directly... use the Ignore struct
     * instead
     */
    int m_selectionLocklevel;
    int m_clipboardLocklevel;

    URLGrabber *m_myURLGrabber;
    QString m_lastURLGrabberTextSelection;
    QString m_lastURLGrabberTextClipboard;
    KSharedConfigPtr m_config;
    QTimer m_overflowClearTimer;
    QTimer m_pendingCheckTimer;
    bool m_pendingContentsCheck;

    bool blockFetchingNewData();
    QString cycleText() const;
    KActionCollection *m_collection;
    QMenu *m_actionsPopup;
    KlipperMode m_mode;
    QTimer *m_saveFileTimer;
    QPointer<KNotification> m_notification;
    KWayland::Client::PlasmaShell *m_plasmashell;
};
