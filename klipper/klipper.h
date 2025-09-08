/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: 2008 Dmitry Suzdalev <dimsuz@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "config-klipper.h"

#include <QClipboard>
#include <QDBusContext>
#include <QElapsedTimer>
#include <QPointer>

#include <KSharedConfig>

#include "klipper_export.h"
#include "urlgrabber.h"

class KToggleAction;
class KActionCollection;
class KlipperPopup;
class HistoryCycler;
class QAction;
class QMenu;
class QMimeData;
class HistoryItem;
class HistoryModel;
class KNotification;
class SystemClipboard;

namespace KWayland::Client
{
class PlasmaShell;
}

class KLIPPER_EXPORT Klipper : public QObject, public QDBusContext
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

    /*
     * Reloads the current configuration
     * @since 6.3
     */
    Q_SCRIPTABLE void reloadConfig();

public:
    static std::shared_ptr<Klipper> self();
    Klipper(QObject *parent = nullptr);
    ~Klipper() override;

    bool eventFilter(QObject *object, QEvent *event) override;

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
        return m_popup.get();
    }

    void showBarcode(std::shared_ptr<const HistoryItem> item);

public Q_SLOTS:
    void saveSession();
    void slotConfigure();
    void slotCycleNext();
    void slotCyclePrev();

Q_SIGNALS:
    void passivePopup(const QString &caption, const QString &text);
    void editFinished(std::shared_ptr<const HistoryItem> item, int result);
    Q_SCRIPTABLE void clipboardHistoryUpdated();

public Q_SLOTS:
    void slotPopupMenu();
    void setURLGrabberEnabled(bool);

protected Q_SLOTS:
    void showPopupMenu(QMenu *);
    void slotRepeatAction();

private Q_SLOTS:
    void slotHistoryChanged(bool isTop = false);

    void slotStartShowTimer();

    void loadSettings();

private:
    static void updateTimestamp();

    std::shared_ptr<SystemClipboard> m_clip;
    HistoryCycler *m_historyCycler = nullptr;

    QElapsedTimer m_showTimer;

    std::shared_ptr<HistoryModel> m_historyModel;
    std::unique_ptr<KlipperPopup> m_popup;

    KToggleAction *m_toggleURLGrabAction;
    QAction *m_clearHistoryAction;
    QAction *m_repeatAction;
    QAction *m_editAction = nullptr;
    QAction *m_showBarcodeAction;
    QAction *m_configureAction;
    QAction *m_quitAction;
    QAction *m_cycleNextAction;
    QAction *m_cyclePrevAction;
    QAction *m_showOnMousePos;

    bool m_bURLGrabber : 1;
    bool m_bReplayActionInHistory : 1;
    bool m_bUseGUIRegExpEditor : 1;

    URLGrabber *m_myURLGrabber;
    QString m_lastURLGrabberTextSelection;
    QString m_lastURLGrabberTextClipboard;

    QString cycleText() const;
    KActionCollection *m_collection;
    QMenu *m_actionsPopup;
    QPointer<KNotification> m_notification;
    KWayland::Client::PlasmaShell *m_plasmashell;
};
