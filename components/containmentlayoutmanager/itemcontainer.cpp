/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemcontainer.h"
#include "configoverlay.h"
#include "containmentlayoutmanager_debug.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTimer>
#include <cmath>

#include <PlasmaQuick/AppletQuickItem>
#include <chrono>

using namespace std::chrono_literals;

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

    Q_EMIT keyChanged();
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
    // if something is grabbing the mouse, make sure to release it.
    // canceling the edit can cause items to be deleted, and Qt doesn't like deleting the item that
    // is currently grabbing the mouse
    if (window() && window()->mouseGrabberItem()) {
        window()->mouseGrabberItem()->ungrabMouse();
    }

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

    if (m_contentItem && (m_editModeCondition != AfterMouseOver || (m_layout && m_layout->editMode()))) {
        m_contentItem->setEnabled(!editMode);
    }

    if (editMode) {
        setZ(1);
    } else {
        setZ(0);
    }

    if (m_mouseDown) {
        sendUngrabRecursive(m_contentItem);
        QMouseEvent ev(QEvent::MouseButtonPress, mapFromScene(m_mouseDownPosition), m_mouseDownPosition, QPointF(), Qt::LeftButton, {}, {});
        ev.setExclusiveGrabber(ev.point(0), this);
        QCoreApplication::sendEvent(this, &ev);
    }

    if (m_dragActive != editMode && m_mouseDown) {
        m_dragActive = editMode && m_mouseDown;
        Q_EMIT dragActiveChanged();
    }

    setConfigOverlayVisible(editMode);

    Q_EMIT editModeChanged(editMode);
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

    Q_EMIT editModeConditionChanged();
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

    Q_EMIT preferredLayoutDirectionChanged();
}

void ItemContainer::setLayout(AppletsLayout *layout)
{
    if (m_layout == layout) {
        return;
    }

    if (m_layout) {
        disconnect(m_layout, &AppletsLayout::editModeConditionChanged, this, nullptr);
        disconnect(m_layout, &AppletsLayout::editModeChanged, this, nullptr);

        if (m_editMode) {
            m_layout->hidePlaceHolder();
        }
    }

    m_layout = layout;

    if (!layout) {
        Q_EMIT layoutChanged();
        return;
    }

    if (parentItem() != layout) {
        setParentItem(layout);
    }

    connect(m_layout, &AppletsLayout::editModeConditionChanged, this, [this]() {
        if (m_layout->editModeCondition() == AppletsLayout::Locked) {
            setEditMode(false);
        }
        if ((m_layout->editModeCondition() == AppletsLayout::Locked) != (m_editModeCondition == ItemContainer::Locked)) {
            Q_EMIT editModeConditionChanged();
        }
    });
    connect(m_layout, &AppletsLayout::editModeChanged, this, [this]() {
        setAcceptHoverEvents(m_editModeCondition == AfterMouseOver || m_layout->editMode());
    });
    Q_EMIT layoutChanged();
}

AppletsLayout *ItemContainer::layout() const
{
    return m_layout;
}

void ItemContainer::onConfigOverlayComponentStatusChanged(QQmlComponent::Status status, QQmlComponent *component)
{
    if (status == QQmlComponent::Loading) {
        return;
    }
    if (!component) {
        component = static_cast<QQmlComponent *>(sender());
    }
    if (status != QQmlComponent::Ready) {
        delete component;
        return;
    }

    Q_ASSERT(!m_configOverlay);
    m_configOverlay = static_cast<ConfigOverlay *>(component->beginCreate(QQmlEngine::contextForObject(this)));

    m_configOverlay->setVisible(false);
    m_configOverlay->setItemContainer(this);
    m_configOverlay->setParentItem(this);
    m_configOverlay->setTouchInteraction(m_mouseSynthetizedFromTouch);
    m_configOverlay->setZ(999);
    m_configOverlay->setPosition(QPointF(0, 0));
    m_configOverlay->setSize(size());

    component->completeCreate();
    component->deleteLater();

    connect(m_configOverlay, &ConfigOverlay::openChanged, this, &ItemContainer::configOverlayVisibleChanged);

    Q_EMIT configOverlayItemChanged();

    m_configOverlay->setOpen(m_configOverlayVisible);
}

void ItemContainer::syncChildItemsGeometry(const QSizeF &size)
{
    if (m_contentItem) {
        m_contentItem->setPosition(QPointF(m_leftPadding, m_topPadding));

        m_contentItem->setSize(QSizeF(size.width() - m_leftPadding - m_rightPadding, size.height() - m_topPadding - m_bottomPadding));
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

QUrl ItemContainer::configOverlaySource() const
{
    return m_configOverlaySource;
}

void ItemContainer::setConfigOverlaySource(const QUrl &url)
{
    if (url == m_configOverlaySource || !url.isValid()) {
        return;
    }

    m_configOverlaySource = url;
    if (m_configOverlay) {
        m_configOverlay->deleteLater();
        m_configOverlay = nullptr;
    }
    Q_EMIT configOverlaySourceChanged();

    if (m_configOverlayVisible) {
        loadConfigOverlayItem();
    }
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

    Q_EMIT initialSizeChanged();
}

bool ItemContainer::configOverlayVisible() const
{
    return m_configOverlay && m_configOverlay->open();
}

void ItemContainer::setConfigOverlayVisible(bool visible)
{
    if (!m_configOverlaySource.isValid() || visible == m_configOverlayVisible) {
        return;
    }

    m_configOverlayVisible = visible;

    if (visible && !m_configOverlay) {
        loadConfigOverlayItem();
    } else if (m_configOverlay) {
        m_configOverlay->setVisible(visible);
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

qsizetype ItemContainer::contentData_count(QQmlListProperty<QObject> *prop)
{
    ItemContainer *container = static_cast<ItemContainer *>(prop->object);
    if (!container) {
        return 0;
    }

    return container->m_contentData.count();
}

QObject *ItemContainer::contentData_at(QQmlListProperty<QObject> *prop, qsizetype index)
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
    return QQmlListProperty<QObject>(this, nullptr, contentData_append, contentData_count, contentData_at, contentData_clear);
}

void ItemContainer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    syncChildItemsGeometry(newGeometry.size());
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    Q_EMIT contentWidthChanged();
    Q_EMIT contentHeightChanged();
}

void ItemContainer::componentComplete()
{
    if (!m_contentItem) {
        // qWarning()<<"Creating default contentItem";
        m_contentItem = new QQuickItem(this);
        syncChildItemsGeometry(size());
    }

    for (auto *o : std::as_const(m_contentData)) {
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
        // NOTE: new syntax cannot be used because we don't have access to the QQuickLayoutAttached class
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

void ItemContainer::loadConfigOverlayItem()
{
    Q_ASSERT(!m_configOverlay);
    constexpr QQmlComponent::CompilationMode mode = QQmlComponent::Asynchronous;
    QQmlContext *context = QQmlEngine::contextForObject(this);
    auto component = new QQmlComponent(context->engine(), context->resolvedUrl(m_configOverlaySource), mode, this);
    if (!component->isLoading()) {
        onConfigOverlayComponentStatusChanged(component->status(), component);
    } else {
        connect(component,
                &QQmlComponent::statusChanged,
                this,
                std::bind(&ItemContainer::onConfigOverlayComponentStatusChanged, this, std::placeholders::_1, nullptr));
    }
}

bool ItemContainer::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    // Don't filter the configoverlay
    if (item == m_configOverlay || (m_configOverlay && m_configOverlay->isAncestorOf(item)) || (!m_editMode && m_editModeCondition == Manual)) {
        if (m_closeEditModeTimer && m_closeEditModeTimer->isActive()) {
            m_closeEditModeTimer->setInterval(2s);
            m_closeEditModeTimer->start();
        }
        return QQuickItem::childMouseEventFilter(item, event);
    }

    // give more time before closing
    if (m_closeEditModeTimer && m_closeEditModeTimer->isActive()) {
        m_closeEditModeTimer->setInterval(500ms);
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
        m_lastMousePosition = me->scenePosition();
        m_mouseDownPosition = me->scenePosition();

        if (m_editMode && !wasEditMode) {
            event->accept();
            return true;
        }

    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (!m_editMode && QPointF(me->scenePosition() - m_mouseDownPosition).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
            m_editModeTimer->stop();
        } else if (m_editMode) {
            event->accept();
        }

    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_editModeTimer->stop();
        m_mouseDown = false;
        m_mouseSynthetizedFromTouch = false;
        if (auto mouseEvent = static_cast<QMouseEvent *>(event); mouseEvent->exclusiveGrabber(mouseEvent->point(0)) == this) {
            mouseEvent->setExclusiveGrabber(mouseEvent->point(0), nullptr);
        }
        event->accept();
        m_dragActive = false;
        if (m_editMode) {
            Q_EMIT dragActiveChanged();
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
        event->setExclusiveGrabber(event->point(0), this);
        setCursor(Qt::ClosedHandCursor);
        m_dragActive = true;
        Q_EMIT dragActiveChanged();
    } else if (m_editModeCondition == AfterPressAndHold) {
        m_editModeTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }

    // Use positions mapped to parent and *not* scene positions because the whole appletsLayout might be zoomed or transformed
    m_lastMousePosition = mapToItem(parentItem(), event->pos());
    m_mouseDownPosition = m_lastMousePosition;
    event->accept();
}

void ItemContainer::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (!m_layout || (!m_editMode && m_editModeCondition == Manual)) {
        return;
    }

    m_mouseDown = false;
    m_mouseSynthetizedFromTouch = false;
    m_editModeTimer->stop();
    if (event->exclusiveGrabber(event->point(0)) == this) {
        event->setExclusiveGrabber(event->point(0), nullptr);
    }

    if (m_editMode && !m_layout->itemIsManaged(this)) {
        m_layout->hidePlaceHolder();
        m_layout->positionItem(this);
    }

    m_dragActive = false;
    if (m_editMode) {
        Q_EMIT dragActiveChanged();
        setCursor(Qt::OpenHandCursor);
    }
    event->accept();
}

void ItemContainer::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->button() == Qt::NoButton && event->buttons() == Qt::NoButton) || (!m_editMode && m_editModeCondition == Manual)) {
        return;
    }

    // Use positions mapped to parent and *not* scene positions because the whole appletsLayout might be zoomed or transformed
    const QPointF parentPos = mapToItem(parentItem(), event->pos());

    if (!m_editMode && QPointF(parentPos - m_mouseDownPosition).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
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
        event->setExclusiveGrabber(event->point(0), this);
        m_dragActive = true;
        Q_EMIT dragActiveChanged();

    } else {
        setPosition(QPointF(x() + parentPos.x() - m_lastMousePosition.x(), y() + parentPos.y() - m_lastMousePosition.y()));

        if (m_layout) {
            m_layout->showPlaceHolderForItem(this);
        }

        Q_EMIT userDrag(QPointF(x(), y()), event->pos());
    }
    m_lastMousePosition = parentPos;
    event->accept();
}

void ItemContainer::mouseUngrabEvent()
{
    m_mouseDown = false;
    m_mouseSynthetizedFromTouch = false;
    m_editModeTimer->stop();

    if (m_layout && m_editMode && !m_layout->itemIsManaged(this)) {
        m_layout->hidePlaceHolder();
        m_layout->positionItem(this);
    }

    m_dragActive = false;
    if (m_editMode) {
        Q_EMIT dragActiveChanged();
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
        connect(m_closeEditModeTimer, &QTimer::timeout, this, [this]() {
            setEditMode(false);
        });
    }
    m_closeEditModeTimer->setInterval(500ms);
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

    item->setVisible(true);
    m_contentItem->setPosition(QPointF(m_leftPadding, m_topPadding));
    m_contentItem->setSize(QSizeF(width() - m_leftPadding - m_rightPadding, height() - m_topPadding - m_bottomPadding));

    Q_EMIT contentItemChanged();
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

    Q_EMIT backgroundChanged();
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
    Q_EMIT leftPaddingChanged();
    Q_EMIT contentWidthChanged();
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
    Q_EMIT topPaddingChanged();
    Q_EMIT contentHeightChanged();
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
    Q_EMIT rightPaddingChanged();
    Q_EMIT contentWidthChanged();
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
    Q_EMIT bottomPaddingChanged();
    Q_EMIT contentHeightChanged();
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
