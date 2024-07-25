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

#include <KSharedConfig>

#include "klipper_export.h"
#include "urlgrabber.h"

class KToggleAction;
class KActionCollection;
class KlipperPopup;
class QTime;
class History;
class QAction;
class QMenu;
class QMimeData;
class HistoryItem;
class HistoryModel;
class KNotification;
class SystemClipboard;

namespace KWayland
{
namespace Client
{
class PlasmaShell;
}
}

class KLIPPER_EXPORT Klipper : public QObject
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
    Klipper(QObject *parent, const KSharedConfigPtr &config);
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

    void showBarcode(std::shared_ptr<const HistoryItem> item);

public Q_SLOTS:
    void saveSession();
    void slotConfigure();
    void slotCycleNext();
    void slotCyclePrev();

protected:
    /**
     * Check data in clipboard, and if it passes these checks,
     * store the data in the clipboard history.
     */
    void checkClipData(QClipboard::Mode mode, const QMimeData *data);

    /**
     * Enter clipboard data in the history.
     */
    [[nodiscard]] std::shared_ptr<HistoryItem> applyClipChanges(const QMimeData *data);

    bool ignoreClipboardChanges() const;

    KSharedConfigPtr config() const
    {
        return m_config;
    }

Q_SIGNALS:
    void passivePopup(const QString &caption, const QString &text);
    void editFinished(std::shared_ptr<const HistoryItem> item, int result);
    Q_SCRIPTABLE void clipboardHistoryUpdated();

public Q_SLOTS:
    void slotIgnored(QClipboard::Mode mode);
    void slotReceivedEmptyClipboard(QClipboard::Mode mode);

    void slotPopupMenu();
    void setURLGrabberEnabled(bool);

protected Q_SLOTS:
    void showPopupMenu(QMenu *);
    void slotRepeatAction();
    void disableURLGrabber();

private Q_SLOTS:
    void slotHistoryChanged();

    void slotStartShowTimer();

    void loadSettings();

private:
    explicit Klipper(const KSharedConfigPtr &config);
    static void updateTimestamp();

    std::shared_ptr<SystemClipboard> m_clip;

    QElapsedTimer m_showTimer;

    History *m_history;
    std::shared_ptr<HistoryModel> m_historyModel;
    KlipperPopup *m_popup;

    KToggleAction *m_toggleURLGrabAction;
    QAction *m_clearHistoryAction;
    QAction *m_repeatAction;
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

    URLGrabber *m_myURLGrabber;
    QString m_lastURLGrabberTextSelection;
    QString m_lastURLGrabberTextClipboard;
    KSharedConfigPtr m_config;

    QString cycleText() const;
    KActionCollection *m_collection;
    QMenu *m_actionsPopup;
    QPointer<KNotification> m_notification;
    KWayland::Client::PlasmaShell *m_plasmashell;
};
