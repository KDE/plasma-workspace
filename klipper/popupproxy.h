/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QRegularExpression>

#include "history.h"

class QMenu;

class HistoryItem;
class KlipperPopup;

/**
 * Proxy helper for the "more" menu item
 *
 */
class PopupProxy : public QObject
{
    Q_OBJECT

public:
    /**
     * Inserts up to itemsPerMenu into parent from parent->youngest(),
     * and spills any remaining items into a more menu.
     */
    PopupProxy(KlipperPopup *parent, int menu_height, int menu_width);

    KlipperPopup *parent();

    /**
     * Called when rebuilding the menu
     * Deletes any More menus.. and start (re)inserting into the toplevel menu.
     * @param index Items are inserted at index.
     * @param filter If non-empty, only insert items that match filter as a regex
     * @return number of items inserted.
     */
    int buildParent(int index, const QRegularExpression &filter = QRegularExpression());

public Q_SLOTS:
    void slotAboutToShow();
    void slotHistoryChanged();

private:
    /**
     * Insert up to m_itemsPerMenu items from spill and a new
     * more-menu if necessary.
     * @param index Items are inserted at index
     * @return number of items inserted.
     */
    int insertFromSpill(int index = 0);

    /**
     * Insert item into proxy_for_menu at index,
     * subtracting the items height from remainingHeight
     */
    void tryInsertItem(HistoryItem const *const item, int &remainingHeight, const int index);

    /**
     * Delete all "More..." menus current created.
     */
    void deleteMoreMenus();

private:
    QMenu *m_proxy_for_menu;
    QByteArray m_spill_uuid;
    QRegularExpression m_filter;
    int m_menu_height;
    int m_menu_width;
};
