/* This file is part of the KDE project
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>

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
#include "popupproxy.h"

#include <QStyle>
#include <QPixmap>
#include <QStyleOption>

#include <KLocalizedString>

#include "historyitem.h"
#include "history.h"
#include "klipperpopup.h"

PopupProxy::PopupProxy( KlipperPopup* parent, int menu_height, int menu_width )
    : QObject( parent ),
      m_proxy_for_menu( parent ),
      m_spill_uuid(),
      m_menu_height( menu_height ),
      m_menu_width( menu_width )
{
    if (!parent->history()->empty()) {
        m_spill_uuid = parent->history()->first()->uuid();
    }
    connect( parent->history(), &History::changed, this, &PopupProxy::slotHistoryChanged );
    connect(m_proxy_for_menu, SIGNAL(triggered(QAction*)), parent->history(), SLOT(slotMoveToTop(QAction*)));
}

void PopupProxy::slotHistoryChanged() {
    deleteMoreMenus();

}

void PopupProxy::deleteMoreMenus() {
    const QMenu* myParent = parent();
    if ( myParent != m_proxy_for_menu ) {
        QMenu* delme = m_proxy_for_menu;
        m_proxy_for_menu = static_cast<QMenu*>( m_proxy_for_menu->parent() );
        while ( m_proxy_for_menu != myParent ) {
            delme = m_proxy_for_menu;
            m_proxy_for_menu = static_cast<QMenu*>( m_proxy_for_menu->parent() );
        }
        // We are called probably from within the menus event-handler (triggered=>slotMoveToTop=>changed=>slotHistoryChanged=>deleteMoreMenus)
        // what can result in a crash if we just delete the menu here (#155196 and #165154) So, delay the delete.
        delme->deleteLater();
    }
}

int PopupProxy::buildParent( int index, const QRegularExpression &filter ) {
    deleteMoreMenus();
    // Start from top of  history (again)
    m_spill_uuid = parent()->history()->empty() ? QByteArray() : parent()->history()->first()->uuid();
    if ( filter.isValid() ) {
        m_filter = filter;
    }

    return insertFromSpill( index );

}

KlipperPopup* PopupProxy::parent() {
    return static_cast<KlipperPopup*>( QObject::parent() );
}

void PopupProxy::slotAboutToShow() {
    insertFromSpill();
}

void PopupProxy::tryInsertItem( HistoryItem const * const item,
                                int& remainingHeight,
                                const int index )
{
    QAction *action = new QAction(m_proxy_for_menu);
    QPixmap image( item->image() );
    if ( image.isNull() ) {
        // Squeeze text strings so that do not take up the entire screen (or more)
        QString text = m_proxy_for_menu->fontMetrics().elidedText( item->text().simplified(), Qt::ElideMiddle, m_menu_width );
        text.replace( QLatin1Char('&'), QLatin1String("&&") );
        action->setText(text);
    } else {
#if 0 // not used because QAction#setIcon does not respect this size; it does scale anyway. TODO: find a way to set a bigger image
        const QSize max_size( m_menu_width,m_menu_height/4 );
        if ( image.height() > max_size.height() || image.width() > max_size.width() ) {
            image = image.scaled( max_size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        }
#endif
        action->setIcon(QIcon(image));
    }

    action->setData(item->uuid());

    // if the m_proxy_for_menu is a submenu (aka a "More" menu) then it may the case, that there is no other action in that menu yet.
    QAction *before = index < m_proxy_for_menu->actions().count() ? m_proxy_for_menu->actions().at(index) : nullptr;
    // insert the new action to the m_proxy_for_menu
    m_proxy_for_menu->insertAction(before, action);

    // Determine height of a menu item.
    QStyleOptionMenuItem style_options;
    // It would be much easier to use QMenu::initStyleOptions. But that is protected, so until we have a better
    // excuse to subclass that, I'd rather implement this manually.
    // Note 2 properties, tabwidth and maxIconWidth, are not available from the public interface, so those are left out (probably not 
    // important for height. Also, Exclusive checkType is disregarded as  I don't think we will ever use it)
    style_options.initFrom(m_proxy_for_menu);
    style_options.checkType = action->isCheckable() ? QStyleOptionMenuItem::NonExclusive : QStyleOptionMenuItem::NotCheckable;
    style_options.checked = action->isChecked();
    style_options.font = action->font();
    style_options.icon = action->icon();
    style_options.menuHasCheckableItems = true;
    style_options.menuRect = m_proxy_for_menu->rect();
    style_options.text = action->text();

    int font_height = QFontMetrics(m_proxy_for_menu->fontMetrics()).height();

    int itemheight = m_proxy_for_menu->style()->sizeFromContents(QStyle::CT_MenuItem,
                                                              &style_options,
                                                              QSize( 0, font_height ),
                                                              m_proxy_for_menu).height();
    // Subtract the used height
    remainingHeight -= itemheight;
}

int PopupProxy::insertFromSpill( int index ) {

    const History* history = parent()->history();
    // This menu is going to be filled, so we don't need the aboutToShow()
    // signal anymore
    disconnect( m_proxy_for_menu, nullptr, this, nullptr );

    // Insert history items into the current m_proxy_for_menu,
    // discarding any that doesn't match the current filter.
    // stop when the total number of items equal m_itemsPerMenu;
    int count = 0;
    int remainingHeight = m_menu_height - m_proxy_for_menu->sizeHint().height();
    auto item = history->find(m_spill_uuid);
    if (!item) {
        return count;
    }
    do {
        if (m_filter.match(item->text()).hasMatch()) {
            tryInsertItem( item.data(), remainingHeight, index++ );
            count++;
        }
        item = history->find(item->next_uuid());
    } while ( item && history->first() != item && remainingHeight >= 0);
    m_spill_uuid = item->uuid();

    // If there is more items in the history, insert a new "More..." menu and
    // make *this a proxy for that menu ('s content).
    if (history->first() && m_spill_uuid != history->first()->uuid()) {
        QMenu* moreMenu = new QMenu(i18n("&More"), m_proxy_for_menu);
        connect(moreMenu, &QMenu::aboutToShow, this, &PopupProxy::slotAboutToShow);
        QAction *before = index < m_proxy_for_menu->actions().count() ? m_proxy_for_menu->actions().at(index) : nullptr;
        m_proxy_for_menu->insertMenu(before, moreMenu);
        m_proxy_for_menu = moreMenu;
    }

    // Return the number of items inserted.
    return count;

}

