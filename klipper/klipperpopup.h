/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QList>

#include <QMenu>

class QAction;
class QWidgetAction;
class QKeyEvent;

class KLineEdit;

class PopupProxy;
class History;

/**
 * Default view of clipboard history.
 *
 */
class KlipperPopup : public QMenu
{
    Q_OBJECT

public:
    explicit KlipperPopup(History *history);
    ~KlipperPopup() override = default;

    /**
     * Normally, the popupmenu is only rebuilt just before showing.
     * If you need the pixel-size or similar of the this menu, call
     * this beforehand.
     */
    void ensureClean();

    History *history()
    {
        return m_history;
    }
    const History *history() const
    {
        return m_history;
    }

public Q_SLOTS:
    void slotHistoryChanged()
    {
        m_dirty = true;
    }
    void slotTopIsUserSelectedSet();
    void slotAboutToShow();
    /**
     * set the top history item active, to easy kb navigation
     */
    void slotSetTopActive();

private:
    void rebuild(const QString &filter = QString());
    void buildFromScratch();
    void showStatus(const QString &errorText);

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void showEvent(QShowEvent *e) override;

private:
    bool m_dirty : 1; // true if menu contents needs to be rebuild.

    /**
     * The "document" (clipboard history)
     */
    History *m_history;

    /**
     * Proxy helper object used to track history items
     */
    PopupProxy *m_popupProxy;

    /**
     * search filter widget
     */
    KLineEdit *m_filterWidget;

    /**
     * Action of search widget
     */
    QWidgetAction *m_filterWidgetAction;

    /**
     * The last event which was received. Used to avoid an infinite event loop
     */
    QKeyEvent *m_lastEvent;
};
