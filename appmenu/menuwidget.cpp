/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#include "menuwidget.h"

#include <QMenu>
#include <QDesktopWidget>
#include <QGraphicsView>
#include <QGraphicsLinearLayout>

#include <KWindowSystem>
#include <KDebug>
#include <KApplication>

MenuWidget::MenuWidget(QGraphicsView *view) :
    QGraphicsWidget(),
    m_mouseTimer(new QTimer(this)),
    m_actionTimer(new QTimer(this)),
    m_view(view),
    m_layout(new QGraphicsLinearLayout(this)),
    m_currentButton(0),
    m_contentBottomMargin(0),
    m_mousePosition(-1, -1),
    m_visibleMenu(0),
    m_menu(0)
{
    connect(m_actionTimer, SIGNAL(timeout()), SLOT(slotUpdateActions()));
    connect(m_mouseTimer, SIGNAL(timeout()), SLOT(slotCheckActiveItem()));
}

MenuWidget::~MenuWidget()
{
    while (!m_buttons.isEmpty()) {
        delete m_buttons.front();
        m_buttons.pop_front();
    }
}

void MenuWidget::setMenu(QMenu *menu)
{
    if (m_menu) {
        disconnect(m_menu, SIGNAL(destroyed()), this, SLOT(slotMenuDestroyed()));
        m_menu->removeEventFilter(this);
    }
    if (menu) {
        if (m_mouseTimer->isActive()) {
            m_mouseTimer->stop();
        }
        m_visibleMenu = 0;
        m_menu = menu;
        connect(m_menu, SIGNAL(destroyed()), SLOT(slotMenuDestroyed()), Qt::UniqueConnection);
        m_menu->installEventFilter(this);
        slotUpdateActions();
    }
}

void MenuWidget::initLayout()
{
    MenuButton* button = 0;

    if (!m_menu) {
        return;
    }

    foreach (QAction* action, m_menu->actions())
    {
        button = createButton(action);
        if (button) {
            m_layout->addItem(button);
            button->setMenu(action->menu());
            m_buttons << button;
        }
    }

    //Assume all buttons have same margins
    if (button) {
        m_contentBottomMargin = button->bottomMargin();
    }
}

bool MenuWidget::eventFilter(QObject* object, QEvent* event)
{
    bool filtered;
    if (object == m_menu) {
        filtered = menuEventFilter(event);
    } else {
        filtered = subMenuEventFilter(static_cast<QMenu*>(object), event);
    }
    return filtered ? true : QGraphicsWidget::eventFilter(object, event);
}

bool MenuWidget::menuEventFilter(QEvent* event)
{
    switch (event->type()) {
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
        // Try to limit layout updates
        m_actionTimer->start(500);
        break;
    default:
        break;
    }
    return false;
}

bool MenuWidget::subMenuEventFilter(QObject* object, QEvent* event)
{
    QMenu *menu = static_cast<QMenu*>(object);

    if (event->type() == QEvent::KeyPress) {
        menu->removeEventFilter(this);
        QApplication::sendEvent(menu, event);
        menu->installEventFilter(this);
        if (!event->isAccepted()) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            switch (keyEvent->key()) {
            case Qt::Key_Left:
                showLeftRightMenu(false);
                break;
            case Qt::Key_Right:
                showLeftRightMenu(true);
                break;
            case Qt::Key_Escape:
                menu->hide();
                break;
            default:
                break;
            }
        }
        return true;
    }
    return false;
}

void MenuWidget::slotMenuDestroyed()
{
    m_menu = 0;
    m_visibleMenu = 0;
    m_currentButton = 0;
}

void MenuWidget::slotCheckActiveItem()
{
    MenuButton* buttonBelow = 0;
    QPoint pos =  m_view->mapFromGlobal(QCursor::pos());
    QGraphicsItem* item = m_view->itemAt(pos);

    if (pos == m_mousePosition) {
        return;
    } else {
        m_mousePosition = pos;
    }

    if (item) {
        buttonBelow = qobject_cast<MenuButton*>(item->toGraphicsObject());
    }

    if (!buttonBelow) {
        return;
    }

    if (buttonBelow != m_currentButton) {
        if (m_currentButton && m_currentButton->nativeWidget()) {
            m_currentButton->nativeWidget()->setDown(false);
            m_currentButton->setHovered(false);
        }
        m_currentButton = buttonBelow;
        if (m_currentButton->nativeWidget()) {
            m_currentButton->nativeWidget()->setDown(true);
        }
        m_visibleMenu = showMenu();
    }
}

void MenuWidget::slotMenuAboutToHide()
{
    if (m_currentButton && m_currentButton->nativeWidget()) {
        m_currentButton->nativeWidget()->setDown(false);
    }

    if (m_mouseTimer->isActive()) {
        m_mouseTimer->stop();
    }
    m_visibleMenu = 0;
    emit aboutToHide();
}

void MenuWidget::slotButtonClicked()
{
    m_currentButton = qobject_cast<MenuButton*>(sender());

    if (m_currentButton && m_currentButton->nativeWidget()) {
        m_currentButton->nativeWidget()->setDown(true);
    }
    m_visibleMenu = showMenu();
    // Start auto navigation after click
    if (!m_mouseTimer->isActive())
        m_mouseTimer->start(100);
}

void MenuWidget::slotUpdateActions()
{
    if (m_visibleMenu) {
        return; // Later
    }

    m_actionTimer->stop();
    m_currentButton = 0;
    foreach (MenuButton *button, m_buttons) {
        disconnect(button, SIGNAL(clicked()), this, SLOT(slotButtonClicked()));
        m_layout->removeItem(button);
        button->hide();
        m_buttons.removeOne(button);
        delete button;
    }
    initLayout();
    // Menu may be empty on application startup
    // slotUpdateActions will be called later by eventFilter()
    if (m_menu && m_menu->actions().length()) {
        emit needResize();
    }
}

void MenuWidget::setActiveAction(QAction *action)
{
    if (!m_menu) {
        return;
    }

    m_currentButton = m_buttons.first();

    if (action) {
        QMenu *menu;
        int i = 0;
        foreach (MenuButton *button, m_buttons) {
            menu = m_menu->actions()[i]->menu();
            if (menu && menu == action->menu()) {
                m_currentButton = button;
                break;
            }
            if (++i >= m_menu->actions().length()) {
                break;
            }
        }
    }
    m_currentButton->nativeWidget()->animateClick();
}

void MenuWidget::hide()
{
    if (m_mouseTimer->isActive()) {
        m_mouseTimer->stop();
    }
    QGraphicsWidget::hide();
}

MenuButton* MenuWidget::createButton(QAction *action)
{
    if( action->isSeparator() || !action->menu() || !action->isVisible()) {
        return 0;
    }

    action->setShortcut(QKeySequence());
    MenuButton *button = new MenuButton(this);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    button->setText(action->text());
    connect(button, SIGNAL(clicked()), SLOT(slotButtonClicked()));
    return button;
}

QMenu* MenuWidget::showMenu()
{
    QMenu *menu = 0;

    if (m_visibleMenu) {
        disconnect(m_visibleMenu, SIGNAL(aboutToHide()), this, SLOT(slotMenuAboutToHide()));
        m_visibleMenu->hide();
    }

    if (m_currentButton && m_menu) {
        menu = m_currentButton->menu();
    }

    // Last chance to get menu
    // Some applications like Firefox have empties menus on layout updates
    // They should populate this menu later but in fact, they use another object
    // So, we check here directly the button name, may fail with menubar with buttons with same name (test apps)
    if (menu && menu->actions().length() == 0) {
        foreach (QAction *action, m_menu->actions()) {
            if (action->text() == m_currentButton->text()) {
                menu = action->menu();
                break;
            }
        }
    }

    if (menu) {
        QPoint globalPos = m_view->mapToGlobal(QPoint(0,0));
        QPointF parentPos =  m_currentButton->mapFromParent(QPoint(0,0));
        QRect screen = KApplication::desktop()->screenGeometry();
        int x = globalPos.x() - parentPos.x();
        int y = globalPos.y() + m_currentButton->size().height() - parentPos.y();

        menu->popup(QPoint(x, y));

        // Fix offscreen menu
        if (menu->size().height() + y > screen.height() + screen.y()) {
            y = globalPos.y() - parentPos.y() - menu->size().height();
            if (menu->size().width() + x > screen.width() + screen.x())
                x = screen.width() + screen.x() - menu->size().width();
            else if (menu->size().width() + x < screen.x())
                x = screen.x();
            menu->move(x, y);
        }

        connect(menu, SIGNAL(aboutToHide()), this, SLOT(slotMenuAboutToHide()));

        installEventFilterForAll(menu, this);
    }
    return menu;
}

void MenuWidget::showLeftRightMenu(bool next)
{
    if (!m_currentButton) {
        return;
    }

    int index = m_buttons.indexOf(m_currentButton);
    if (index == -1) {
        kWarning() << "Couldn't find button!";
        return;
    }
    if (next) {
        index = (index + 1) % m_buttons.count();
    } else {
        index = (index == 0 ? m_buttons.count() : index) - 1;
    }

    if (m_currentButton && m_currentButton->nativeWidget()) {
        m_currentButton->nativeWidget()->setDown(false);
    }
    m_currentButton = m_buttons.at(index);
    if (m_currentButton && m_currentButton->nativeWidget()) {
        m_currentButton->nativeWidget()->setDown(true);
    }
    m_visibleMenu = showMenu();
}

void MenuWidget::installEventFilterForAll(QMenu *menu, QObject *object)
{
    if (!menu) {
        return;
    }

    menu->installEventFilter(this);

    foreach (QAction *action, menu->actions()) {
        if (action->menu())
            installEventFilterForAll(action->menu(), object);
    }
}

#include "menuwidget.moc"