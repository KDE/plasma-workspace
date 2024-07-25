/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "popupproxy.h"

#include <QPixmap>
#include <QStyle>
#include <QStyleOption>

#include <KLocalizedString>

#include "historyitem.h"
#include "historymodel.h"
#include "klipperpopup.h"
#include "utils.h"

PopupProxy::PopupProxy(KlipperPopup *parent, int menu_height, int menu_width)
    : QObject(parent)
    , m_model(HistoryModel::self())
    , m_proxy_for_menu(parent)
    , m_spill_uuid()
    , m_menu_height(menu_height)
    , m_menu_width(menu_width)
{
    if (m_model->rowCount() > 0) {
        m_spill_uuid = m_model->first()->uuid();
    }
    connect(m_model.get(), &HistoryModel::changed, this, &PopupProxy::slotHistoryChanged);
    connect(m_proxy_for_menu, &QMenu::triggered, m_model.get(), [this](QAction *action) {
        QByteArray uuid = action->data().toByteArray();
        if (uuid.isNull()) { // not an action from popupproxy
            return;
        }
        m_model->moveToTop(uuid);
    });
}

void PopupProxy::slotHistoryChanged()
{
    deleteMoreMenus();
}

void PopupProxy::deleteMoreMenus()
{
    const QMenu *myParent = parent();
    if (myParent != m_proxy_for_menu) {
        QMenu *delme = m_proxy_for_menu;
        m_proxy_for_menu = static_cast<QMenu *>(m_proxy_for_menu->parent());
        while (m_proxy_for_menu != myParent) {
            delme = m_proxy_for_menu;
            m_proxy_for_menu = static_cast<QMenu *>(m_proxy_for_menu->parent());
        }
        // We are called probably from within the menus event-handler (triggered=>slotMoveToTop=>changed=>slotHistoryChanged=>deleteMoreMenus)
        // what can result in a crash if we just delete the menu here (#155196 and #165154) So, delay the delete.
        delme->deleteLater();
    }
}

int PopupProxy::buildParent(int index, const QRegularExpression &filter)
{
    deleteMoreMenus();
    // Start from top of  history (again)
    m_spill_uuid = m_model->rowCount() == 0 ? QByteArray() : m_model->first()->uuid();
    if (filter.isValid()) {
        m_filter = filter;
    }

    return insertFromSpill(index);
}

KlipperPopup *PopupProxy::parent()
{
    return static_cast<KlipperPopup *>(QObject::parent());
}

void PopupProxy::slotAboutToShow()
{
    insertFromSpill();
}

void PopupProxy::tryInsertItem(HistoryItem const *const item, int &remainingHeight, const int index)
{
    QAction *action = new QAction(m_proxy_for_menu);
    if (QImage image(item->image()); image.isNull()) {
        // Squeeze text strings so that do not take up the entire screen (or more)
        QString text = m_proxy_for_menu->fontMetrics().elidedText(Utils::simplifiedText(item->text(), 1000), Qt::ElideRight, m_menu_width);
        text.replace(QLatin1Char('&'), QLatin1String("&&"));
        action->setText(text);
    } else {
        action->setIcon(QIcon(QPixmap::fromImage(std::move(image))));
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

    int itemheight = m_proxy_for_menu->style()->sizeFromContents(QStyle::CT_MenuItem, &style_options, QSize(0, font_height), m_proxy_for_menu).height();
    // Subtract the used height
    remainingHeight -= itemheight;
}

int PopupProxy::insertFromSpill(int index)
{
    // This menu is going to be filled, so we don't need the aboutToShow()
    // signal anymore
    disconnect(m_proxy_for_menu, nullptr, this, nullptr);

    // Insert history items into the current m_proxy_for_menu,
    // discarding any that doesn't match the current filter.
    // stop when the total number of items equal m_itemsPerMenu;
    int count = 0;
    int remainingHeight = m_menu_height - m_proxy_for_menu->sizeHint().height();
    const int _index = m_model->indexOf(m_spill_uuid);
    if (_index < 0) {
        return count;
    }

    auto item = m_model->index(_index, 0).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
    do {
        if (m_filter.match(item->text()).hasMatch()) {
            tryInsertItem(item.get(), remainingHeight, index++);
            count++;
        }
        item = m_model->index(m_model->indexOf(item->next_uuid()), 0).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
    } while (item && m_model->first() != item && remainingHeight >= 0);
    m_spill_uuid = item->uuid();

    // If there is more items in the history, insert a new "More..." menu and
    // make *this a proxy for that menu ('s content).
    if (m_model->rowCount() > 0 && m_spill_uuid != m_model->first()->uuid()) {
        QMenu *moreMenu = new QMenu(i18n("&More"), m_proxy_for_menu);
        connect(moreMenu, &QMenu::aboutToShow, this, &PopupProxy::slotAboutToShow);
        QAction *before = index < m_proxy_for_menu->actions().count() ? m_proxy_for_menu->actions().at(index) : nullptr;
        m_proxy_for_menu->insertMenu(before, moreMenu);
        m_proxy_for_menu = moreMenu;
    }

    // Return the number of items inserted.
    return count;
}
