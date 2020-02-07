/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>
   Copyright (C) by Andrew Stanley-Jones
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef KLIPPERPOPUP_H
#define KLIPPERPOPUP_H

#include <QList>

#include <QMenu>

class QAction;
class QWidgetAction;
class QKeyEvent;

class KHelpMenu;
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
    explicit KlipperPopup( History* history );
    ~KlipperPopup() override;
    void plugAction( QAction* action );

    /**
     * Normally, the popupmenu is only rebuilt just before showing.
     * If you need the pixel-size or similar of the this menu, call
     * this beforehand.
     */
    void ensureClean();

    History* history() { return m_history; }
    const History* history() const { return m_history; }

    void setShowHelp(bool show) {
        m_showHelp = show;
    }
public Q_SLOTS:
    void slotHistoryChanged() { m_dirty = true; }
    void slotTopIsUserSelectedSet();
    void slotAboutToShow();
    /**
     * set the top history item active, to easy kb navigation
     */
    void slotSetTopActive();

private:
    void rebuild( const QString& filter = QString() );
    void buildFromScratch();

protected:
     void keyPressEvent( QKeyEvent* e ) override;

private:
    bool m_dirty : 1; // true if menu contents needs to be rebuild.

    /**
     * Contains the string shown if the menu is empty.
     */
    QString m_textForEmptyHistory;

    /**
     * Contains the string shown if the search string has no
     * matches and the menu is not empty.
     */
    QString m_textForNoMatch;

    /**
     * The "document" (clipboard history)
     */
    History* m_history;

    /**
     * The help menu
     */
    KHelpMenu* m_helpMenu;

    /**
     * (unowned) actions to plug into the primary popup menu
     */
    QList<QAction*> m_actions;

    /**
     * Proxy helper object used to track history items
     */
    PopupProxy* m_popupProxy;

    /**
     * search filter widget
     */
    KLineEdit* m_filterWidget;

    /**
     * Action of search widget
     */
    QWidgetAction* m_filterWidgetAction;

    /**
     * The current number of history items in the clipboard
     */
    int m_nHistoryItems;

    bool m_showHelp;

    /**
     * The last event which was received. Used to avoid an infinite event loop
     */
    QKeyEvent* m_lastEvent;

};

#endif
