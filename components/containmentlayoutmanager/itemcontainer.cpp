/*
 *   Copyright 2019 by Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "itemcontainer.h"
#include "configoverlay.h"

#include <cmath>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QGuiApplication>
#include <QStyleHints>

#include <PlasmaQuick/AppletQuickItem>

ItemContainer::ItemContainer(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFiltersChildMouseEvents(true);
    setFlags(QQuickItem::ItemIsFocusScope);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    setLayout(qobject_cast<AppletsLayout *>(parent));

    m_editModeTimer = new QTimer(this);
    m_editModeTimer->setSingleShot(true);

    connect(this, &QQuickItem::parentChanged, this, [this]() {
        setLayout(qobject_cast<AppletsLayout *>(parentItem()));
    });

    connect(m_editModeTimer, &QTimer::timeout, this, [this]() {
        setEditMode(true);
    });


    setKeepMouseGrab(true);
    m_sizeHintAdjustTimer = new QTimer(this);
    m_sizeHintAdjustTimer->setSingleShot(true);
    m_sizeHintAdjustTimer->setInterval(0);

    connect(m_sizeHintAdjustTimer, &QTimer::timeout, this, &ItemContainer::sizeHintsChanged);
}

ItemContainer::~ItemContainer()
{
    disconnect(this, &QQuickItem::parentChanged, this, nullptr);

    if (m_contentItem) {
        m_contentItem->setEnabled(true);
    }
}

QString ItemContainer::key() const
{
    return m_key;
}

void ItemContainer::setKey(const QString &key)
{
    if (m_key == key) {
        return;
    }

    m_key = key;

    emit keyChanged();
}

bool ItemContainer::editMode() const
{
    return m_editMode;
}

bool ItemContainer::dragActive() const
{
    return m_dragActive;
}

void ItemContainer::cancelEdit()
{
    m_editModeTimer->stop();
    m_mouseDown = false;
    setEditMode(false);
}

void ItemContainer::setEditMode(bool editMode)
{
    if (m_editMode == editMode) {
        return;
    }

    if (editMode && editModeCondition() == Locked) {
        return;
    }

    m_editMode = editMode;

    if (m_editModeCondition != AfterMouseOver || (m_layout && m_layout->editMode())) {
        m_contentItem->setEnabled(!editMode);
    }

    if (editMode) {
        setZ(1);
    } else {
        setZ(0);
    }

    if (m_mouseDown) {
        sendUngrabRecursive(m_contentItem);
        grabMouse();
    }

    if (m_dragActive != editMode && m_mouseDown) {
        m_dragActive = editMode && m_mouseDown;
        emit dragActiveChanged();
    }

    setConfigOverlayVisible(editMode);

    emit editModeChanged(editMode);
}

ItemContainer::EditModeCondition ItemContainer::editModeCondition() const
{
    if (m_layout && m_layout->editModeCondition() == AppletsLayout::Locked) {
        return Locked;
    }

    return m_editModeCondition;
}

void ItemContainer::setEditModeCondition(EditModeCondition condition)
{
    if (condition == m_editModeCondition) {
        return;
    }

    if (condition == Locked) {
        setEditMode(false);
    }

    m_editModeCondition = condition;

    setAcceptHoverEvents(condition == AfterMouseOver || (m_layout && m_layout->editMode()));

    emit editModeConditionChanged();
}

AppletsLayout::PreferredLayoutDirection ItemContainer::preferredLayoutDirection() const
{
    return m_preferredLayoutDirection;
}

void ItemContainer::setPreferredLayoutDirection(AppletsLayout::PreferredLayoutDirection direction)
{
    if (direction == m_preferredLayoutDirection) {
        return;
    }

    m_preferredLayoutDirection = direction;

    emit preferredLayoutDirectionChanged();
}

void ItemContainer::setLayout(AppletsLayout *layout)
{
    if (m_layout == layout) {
        return;
    }

    if (m_layout) {
        disconnect(m_layout, &AppletsLayout::editModeConditionChanged, this,  nullptr);
        disconnect(m_layout, &AppletsLayout::editModeChanged, this,  nullptr);

        if (m_editMode) {
            m_layout->hidePlaceHolder();
        }
    }

    m_layout = layout;

    if (!layout) {
        emit layoutChanged();
        return;
    }

    if (parentItem() != layout) {
        setParentItem(layout);
    }

    connect(m_layout, &AppletsLayout::editModeConditionChanged, this, [this]() {
        if (m_layout->editModeCondition() == AppletsLayout::Locked) {
            setEditMode(false);
        }
        if ((m_layout->editModeCondition() == AppletsLayout::Locked) !=
            (m_editModeCondition == ItemContainer::Locked)) {
            emit editModeConditionChanged();
        }
    });
    connect(m_layout, &AppletsLayout::editModeChanged, this, [this]() {
        setAcceptHoverEvents(m_editModeCondition == AfterMouseOver || m_layout->editMode());
    });
    emit layoutChanged();
}

AppletsLayout *ItemContainer::layout() const
{
    return m_layout;
}

void ItemContainer::syncChildItemsGeometry(const QSizeF &size)
{
    if (m_contentItem) {
        m_contentItem->setPosition(QPointF(m_leftPadding, m_topPadding));

        m_contentItem->setSize(QSizeF(size.width() - m_leftPadding - m_rightPadding,
                size.height() - m_topPadding - m_bottomPadding));
    }

    if (m_backgroundItem) {
        m_backgroundItem->setPosition(QPointF(0, 0));
        m_backgroundItem->setSize(size);
    }

    if (m_configOverlay) {
        m_configOverlay->setPosition(QPointF(0, 0));
        m_configOverlay->setSize(size);
    }
}

QQmlComponent *ItemContainer::configOverlayComponent() const
{
    return m_configOverlayComponent;
}

void ItemContainer::setConfigOverlayComponent(QQmlComponent *component)
{
    if (component == m_configOverlayComponent) {
        return;
    }

    m_configOverlayComponent = component;
    if (m_configOverlay) {
        m_configOverlay->deleteLater();
        m_configOverlay = nullptr;
    }

    emit configOverlayComponentChanged();
}

ConfigOverlay *ItemContainer::configOverlayItem() const
{
    return m_configOverlay;
}

QSizeF ItemContainer::initialSize() const
{
    return m_initialSize;
}

void ItemContainer::setInitialSize(const QSizeF &size)
{
    if (m_initialSize == size) {
        return;
    }

    m_initialSize = size;

    emit initialSizeChanged();
}

bool ItemContainer::configOverlayVisible() const
{
    return m_configOverlay && m_configOverlay->open();
}

void ItemContainer::setConfigOverlayVisible(bool visible)
{
    if (!m_configOverlayComponent) {
        return;
    }

    if (visible == configOverlayVisible()) {
        return;
    }

    if (visible && !m_configOverlay) {
        QQmlContext *context = QQmlEngine::contextForObject(this);
        Q_ASSERT(context);
        QObject *instance = m_configOverlayComponent->beginCreate(context);
        m_configOverlay = qobject_cast<ConfigOverlay *>(instance);

        if (!m_configOverlay) {
            qWarning() << "Error: Applet configOverlay not of ConfigOverlay type";
            if (instance) {
                instance->deleteLater();
            }
            return;
        }

        m_configOverlay->setVisible(false);
        m_configOverlay->setItemContainer(this);
        m_configOverlay->setParentItem(this);
        m_configOverlay->setTouchInteraction(m_mouseSynthetizedFromTouch);
        m_configOverlay->setZ(999);
        m_configOverlay->setPosition(QPointF(0, 0));
        m_configOverlay->setSize(size());

        m_configOverlayComponent->completeCreate();

        connect(m_configOverlay, &ConfigOverlay::openChanged, this, [this]() {
            emit configOverlayVisibleChanged(m_configOverlay->open());
        });

        emit configOverlayItemChanged();
    }

    if (m_configOverlay) {
        m_configOverlay->setOpen(visible);
    }
}

void ItemContainer::contentData_append(QQmlListProperty<QObject> *prop, QObject *object)
{
    ItemContainer *container = static_cast<ItemContainer *>(prop->object);
    if (!container) {
        return;
    }

//    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    container->m_contentData.append(object);
}

int ItemContainer::contentData_count(QQmlListProperty<QObject> *prop)
{
    ItemContainer *container = static_cast<ItemContainer *>(prop->object);
    if (!container) {
        return 0;
    }

    return container->m_contentData.count();
}

QObject *ItemContainer::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    ItemContainer *container = static_cast<ItemContainer *>(prop->object);
    if (!container) {
        return nullptr;
    }

    if (index < 0 || index >= container->m_contentData.count()) {
        return nullptr;
    }
    return container->m_contentData.value(index);
}

void ItemContainer::contentData_clear(QQmlListProperty<QObject> *prop)
{
    ItemContainer *container = static_cast<ItemContainer *>(prop->object);
    if (!container) {
        return;
    }

    return container->m_contentData.clear();
}

QQmlListProperty<QObject> ItemContainer::contentData()
{
    return QQmlListProperty<QObject>(this, nullptr,
                                     contentData_append,
                                     contentData_count,
                                     contentData_at,
                                     contentData_clear);
}

void ItemContainer::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    syncChildItemsGeometry(newGeometry.size());
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
    emit contentWidthChanged();
    emit contentHeightChanged();
}

void ItemContainer::componentComplete()
{
    if (!m_contentItem) {
        //qWarning()<<"Creating default contentItem";
        m_contentItem = new QQuickItem(this);
        syncChildItemsGeometry(size());
    }

    for (auto *o : m_contentData) {
        QQuickItem *item = qobject_cast<QQuickItem *>(o);
        if (item) {
            item->setParentItem(m_contentItem);
        }
    }

    // Search for the Layout attached property
    // Qt6: this should become public api
    // https://bugreports.qt.io/browse/QTBUG-77103
    for (auto *o : children()) {
        if (o->inherits("QQuickLayoutAttached")) {
            m_layoutAttached = o;
        }
    }

    if (m_layoutAttached) {
        //NOTE: new syntax cannot be used because we don't have access to the QQuickLayoutAttached class
        connect(m_layoutAttached, SIGNAL(minimumHeightChanged()), m_sizeHintAdjustTimer, SLOT(start()));
        connect(m_layoutAttached, SIGNAL(minimumWidthChanged()), m_sizeHintAdjustTimer, SLOT(start()));
    
        connect(m_layoutAttached, SIGNAL(preferredHeightChanged()), m_sizeHintAdjustTimer, SLOT(start()));
        connect(m_layoutAttached, SIGNAL(preferredWidthChanged()), m_sizeHintAdjustTimer, SLOT(start()));

        connect(m_layoutAttached, SIGNAL(maximumHeightChanged()), m_sizeHintAdjustTimer, SLOT(start()));
        connect(m_layoutAttached, SIGNAL(maximumWidthChanged()), m_sizeHintAdjustTimer, SLOT(start()));
    }
    QQuickItem::componentComplete();
}

void ItemContainer::sendUngrabRecursive(QQuickItem *item)
{
    if (!item || !item->window()) {
        return;
    }

    for (auto *child : item->childItems()) {
        sendUngrabRecursive(child);
    }

    QEvent ev(QEvent::UngrabMouse);

    QCoreApplication::sendEvent(item, &ev);
}

bool ItemContainer::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    // Don't filter the configoverlay
    if (item == m_configOverlay
        || (m_configOverlay && m_configOverlay->isAncestorOf(item))
        || (!m_editMode && m_editModeCondition == Manual)) {
        return QQuickItem::childMouseEventFilter(item, event);
    }

    //give more time before closing
    if (m_closeEditModeTimer && m_closeEditModeTimer->isActive()) {
        m_closeEditModeTimer->start();
    }
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton && !(me->buttons() & Qt::LeftButton)) {
            return QQuickItem::childMouseEventFilter(item, event);
        }
        forceActiveFocus(Qt::MouseFocusReason);
        m_mouseDown = true;
        m_mouseSynthetizedFromTouch = me->source() == Qt::MouseEventSynthesizedBySystem || me->source() == Qt::MouseEventSynthesizedByQt;
        if (m_configOverlay) {
            m_configOverlay->setTouchInteraction(m_mouseSynthetizedFromTouch);
        }

        const bool wasEditMode = m_editMode;
        if (m_layout && m_layout->editMode()) {
            setEditMode(true);
        } else if (m_editModeCondition == AfterPressAndHold) {
            m_editModeTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
        }
        m_lastMousePosition = me->windowPos();
        m_mouseDownPosition = me->windowPos();

        if (m_editMode && !wasEditMode) {
            event->accept();
            return true;
        }

    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (!m_editMode
        && QPointF(me->windowPos() - m_mouseDownPosition).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
            m_editModeTimer->stop();
        } else if (m_editMode) {
            event->accept();
        }

    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_editModeTimer->stop();
        m_mouseDown = false;
        m_mouseSynthetizedFromTouch = false;
        ungrabMouse();
        event->accept();
        m_dragActive = false;
        if (m_editMode) {
            emit dragActiveChanged();
        }
    }

    return QQuickItem::childMouseEventFilter(item, event);
}

void ItemContainer::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus(Qt::MouseFocusReason);

    if (!m_editMode && m_editModeCondition == Manual) {
        return;
    }

    m_mouseDown = true;
    m_mouseSynthetizedFromTouch = event->source() == Qt::MouseEventSynthesizedBySystem || event->source() == Qt::MouseEventSynthesizedByQt;
    if (m_configOverlay) {
        m_configOverlay->setTouchInteraction(m_mouseSynthetizedFromTouch);
    }

    if (m_layout && m_layout->editMode()) {
        setEditMode(true);
    }

    if (m_editMode) {
        grabMouse();
        setCursor(Qt::ClosedHandCursor);
        m_dragActive = true;
        emit dragActiveChanged();
    } else if (m_editModeCondition == AfterPressAndHold) {
        m_editModeTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }

    m_lastMousePosition = event->windowPos();
    m_mouseDownPosition = event->windowPos();
    event->accept();
}

void ItemContainer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (!m_layout
        || (!m_editMode && m_editModeCondition == Manual)) {
        return;
    }

    m_mouseDown = false;
    m_mouseSynthetizedFromTouch = false;
    m_editModeTimer->stop();
    ungrabMouse();

    if (m_editMode && !m_layout->itemIsManaged(this)) {
        m_layout->hidePlaceHolder();
        m_layout->positionItem(this);
    }

    m_dragActive = false;
    if (m_editMode) {
        emit dragActiveChanged();
        setCursor(Qt::OpenHandCursor);
    }
    event->accept();
}

void ItemContainer::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::NoButton && event->buttons() == Qt::NoButton)
        || (!m_editMode && m_editModeCondition == Manual)) {
        return;
    }

    if (!m_editMode
        && QPointF(event->windowPos() - m_mouseDownPosition).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
        if (m_editModeCondition == AfterPress) {
            setEditMode(true);
        } else {
            m_editModeTimer->stop();
        }
    }

    if (!m_editMode) {
        return;
    }

    if (m_layout && m_layout->itemIsManaged(this)) {
        m_layout->releaseSpace(this);
        grabMouse();
        m_dragActive = true;
        emit dragActiveChanged();

    } else {
        setPosition(QPointF(x() + event->windowPos().x() - m_lastMousePosition.x(),
                            y() + event->windowPos().y() - m_lastMousePosition.y()));

        if (m_layout) {
            m_layout->showPlaceHolderForItem(this);
        }
        
        emit userDrag(QPointF(x(), y()), event->pos());
    }
    m_lastMousePosition = event->windowPos();
    event->accept();
}

void ItemContainer::mouseUngrabEvent()
{
    m_mouseDown = false;
    m_mouseSynthetizedFromTouch = false;
    m_editModeTimer->stop();
    ungrabMouse();

    if (m_layout && m_editMode && !m_layout->itemIsManaged(this)) {
        m_layout->hidePlaceHolder();
        m_layout->positionItem(this);
    }

    m_dragActive = false;
    if (m_editMode) {
        emit dragActiveChanged();
    }
}

void ItemContainer::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event);

    if (m_editModeCondition != AfterMouseOver && !m_layout->editMode()) {
        return;
    }

    if (m_closeEditModeTimer) {
        m_closeEditModeTimer->stop();
    }

    if (m_layout->editMode()) {
        setCursor(Qt::OpenHandCursor);
        setEditMode(true);
    } else {
        m_editModeTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }
}

void ItemContainer::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event);

    if (m_editModeCondition != AfterMouseOver && !m_layout->editMode()) {
        return;
    }

    m_editModeTimer->stop();
    if (!m_closeEditModeTimer) {
        m_closeEditModeTimer = new QTimer(this);
        m_closeEditModeTimer->setSingleShot(true);
        m_closeEditModeTimer->setInterval(500);
        connect(m_closeEditModeTimer, &QTimer::timeout, this, [this] () {
            setEditMode(false);
        });
    }
    m_closeEditModeTimer->start();
}

QQuickItem *ItemContainer::contentItem() const
{
    return m_contentItem;
}

void ItemContainer::setContentItem(QQuickItem *item)
{
    if (m_contentItem == item) {
        return;
    }

    m_contentItem = item;
    item->setParentItem(this);
    m_contentItem->setPosition(QPointF(m_leftPadding, m_topPadding));

    m_contentItem->setSize(QSizeF(width() - m_leftPadding - m_rightPadding,
            height() - m_topPadding - m_bottomPadding));

    emit contentItemChanged();
}

QQuickItem *ItemContainer::background() const
{
    return m_backgroundItem;
}

void ItemContainer::setBackground(QQuickItem *item)
{
    if (m_backgroundItem == item) {
        return;
    }

    m_backgroundItem = item;
    m_backgroundItem->setParentItem(this);
    m_backgroundItem->setPosition(QPointF(0, 0));
    m_backgroundItem->setSize(size());

    emit backgroundChanged();
}

int ItemContainer::leftPadding() const
{
    return m_leftPadding;
}

void ItemContainer::setLeftPadding(int padding)
{
    if (m_leftPadding == padding) {
        return;
    }

    m_leftPadding = padding;
    syncChildItemsGeometry(size());
    emit leftPaddingChanged();
    emit contentWidthChanged();
}


int ItemContainer::topPadding() const
{
    return m_topPadding;
}

void ItemContainer::setTopPadding(int padding)
{
    if (m_topPadding == padding) {
        return;
    }

    m_topPadding = padding;
    syncChildItemsGeometry(size());
    emit topPaddingChanged();
    emit contentHeightChanged();
}


int ItemContainer::rightPadding() const
{
    return m_rightPadding;
}

void ItemContainer::setRightPadding(int padding)
{
    if (m_rightPadding == padding) {
        return;
    }

    m_rightPadding = padding;
    syncChildItemsGeometry(size());
    emit rightPaddingChanged();
    emit contentWidthChanged();
}


int ItemContainer::bottomPadding() const
{
    return m_bottomPadding;
}

void ItemContainer::setBottomPadding(int padding)
{
    if (m_bottomPadding == padding) {
        return;
    }

    m_bottomPadding = padding;
    syncChildItemsGeometry(size());
    emit bottomPaddingChanged();
    emit contentHeightChanged();
}

int ItemContainer::contentWidth() const
{
    return width() - m_leftPadding - m_rightPadding;
}

int ItemContainer::contentHeight() const
{
    return height() - m_topPadding - m_bottomPadding;
}

#include "moc_itemcontainer.cpp"
