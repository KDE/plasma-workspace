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

#include "appletslayout.h"
#include "appletcontainer.h"
#include "gridlayoutmanager.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QStyleHints>
#include <QTimer>

// Plasma
#include <Containment>
#include <Corona>
#include <PlasmaQuick/AppletQuickItem>

AppletsLayout::AppletsLayout(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_layoutManager = new GridLayoutManager(this);

    setFlags(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);

    m_saveLayoutTimer = new QTimer(this);
    m_saveLayoutTimer->setSingleShot(true);
    m_saveLayoutTimer->setInterval(100);
    connect(m_layoutManager, &AbstractLayoutManager::layoutNeedsSaving, m_saveLayoutTimer, QOverload<>::of(&QTimer::start));
    connect(m_saveLayoutTimer, &QTimer::timeout, this, [this]() {
        if (!m_configKey.isEmpty() && m_containment && m_containment->corona()->isStartupCompleted()) {
            const QString serializedConfig = m_layoutManager->serializeLayout();
            m_containment->config().writeEntry(m_configKey, serializedConfig);
            // FIXME: something more efficient
            m_layoutManager->parseLayout(serializedConfig);
            m_savedSize = size();
            m_containment->corona()->requireConfigSync();
        }
    });

    m_configKeyChangeTimer = new QTimer(this);
    m_configKeyChangeTimer->setSingleShot(true);
    m_configKeyChangeTimer->setInterval(100);
    connect(m_configKeyChangeTimer, &QTimer::timeout, this, [this]() {
        if (!m_configKey.isEmpty() && m_containment) {
            m_layoutManager->parseLayout(m_containment->config().readEntry(m_configKey, ""));

            if (width() > 0 && height() > 0) {
                m_layoutManager->resetLayoutFromConfig();
                m_savedSize = size();
            }
        }
    });
    m_pressAndHoldTimer = new QTimer(this);
    m_pressAndHoldTimer->setSingleShot(true);
    connect(m_pressAndHoldTimer, &QTimer::timeout, this, [this]() {
        setEditMode(true);
    });

    m_sizeSyncTimer = new QTimer(this);
    m_sizeSyncTimer->setSingleShot(true);
    m_sizeSyncTimer->setInterval(150);
    connect(m_sizeSyncTimer, &QTimer::timeout, this, [this]() {
        const QRect newGeom(x(), y(), width(), height());
        // The size has been restored from the last one it has been saved: restore that exact same layout
        if (newGeom.size() == m_savedSize) {
            m_layoutManager->resetLayoutFromConfig();

            // If the resize is consequence of a screen resolution change, queue a relayout maintaining the distance between screen edges
        } else if (!m_geometryBeforeResolutionChange.isEmpty()) {
            m_layoutManager->layoutGeometryChanged(newGeom, m_geometryBeforeResolutionChange);
            m_geometryBeforeResolutionChange = QRectF();

            // Heuristically relayout items only when the plasma startup is fully completed
        } else {
            polish();
        }
    });
}

AppletsLayout::~AppletsLayout()
{
}

PlasmaQuick::AppletQuickItem *AppletsLayout::containment() const
{
    return m_containmentItem;
}

void AppletsLayout::setContainment(PlasmaQuick::AppletQuickItem *containmentItem)
{
    // Forbid changing containmentItem at runtime
    if (m_containmentItem || containmentItem == m_containmentItem || !containmentItem->applet() || !containmentItem->applet()->isContainment()) {
        qWarning() << "Error: cannot change the containment to AppletsLayout";
        return;
    }

    // Can't assign containments that aren't parents
    QQuickItem *candidate = parentItem();
    while (candidate) {
        if (candidate == m_containmentItem) {
            break;
        }
        candidate = candidate->parentItem();
    }
    if (candidate != m_containmentItem) {
        return;
    }

    m_containmentItem = containmentItem;
    m_containment = static_cast<Plasma::Containment *>(m_containmentItem->applet());

    connect(m_containmentItem, SIGNAL(appletAdded(QObject *, int, int)), this, SLOT(appletAdded(QObject *, int, int)));

    connect(m_containmentItem, SIGNAL(appletRemoved(QObject *)), this, SLOT(appletRemoved(QObject *)));

    emit containmentChanged();
}

QString AppletsLayout::configKey() const
{
    return m_configKey;
}

void AppletsLayout::setConfigKey(const QString &key)
{
    if (m_configKey == key) {
        return;
    }

    m_configKey = key;

    // Reloading everything from the new config is expansive, event compress it
    m_configKeyChangeTimer->start();

    emit configKeyChanged();
}

QJSValue AppletsLayout::acceptsAppletCallback() const
{
    return m_acceptsAppletCallback;
}

qreal AppletsLayout::minimumItemWidth() const
{
    return m_minimumItemSize.width();
}

void AppletsLayout::setMinimumItemWidth(qreal width)
{
    if (qFuzzyCompare(width, m_minimumItemSize.width())) {
        return;
    }

    m_minimumItemSize.setWidth(width);

    emit minimumItemWidthChanged();
}

qreal AppletsLayout::minimumItemHeight() const
{
    return m_minimumItemSize.height();
}

void AppletsLayout::setMinimumItemHeight(qreal height)
{
    if (qFuzzyCompare(height, m_minimumItemSize.height())) {
        return;
    }

    m_minimumItemSize.setHeight(height);

    emit minimumItemHeightChanged();
}

qreal AppletsLayout::defaultItemWidth() const
{
    return m_defaultItemSize.width();
}

void AppletsLayout::setDefaultItemWidth(qreal width)
{
    if (qFuzzyCompare(width, m_defaultItemSize.width())) {
        return;
    }

    m_defaultItemSize.setWidth(width);

    emit defaultItemWidthChanged();
}

qreal AppletsLayout::defaultItemHeight() const
{
    return m_defaultItemSize.height();
}

void AppletsLayout::setDefaultItemHeight(qreal height)
{
    if (qFuzzyCompare(height, m_defaultItemSize.height())) {
        return;
    }

    m_defaultItemSize.setHeight(height);

    emit defaultItemHeightChanged();
}

qreal AppletsLayout::cellWidth() const
{
    return m_layoutManager->cellSize().width();
}

void AppletsLayout::setCellWidth(qreal width)
{
    if (qFuzzyCompare(width, m_layoutManager->cellSize().width())) {
        return;
    }

    m_layoutManager->setCellSize(QSizeF(width, m_layoutManager->cellSize().height()));

    emit cellWidthChanged();
}

qreal AppletsLayout::cellHeight() const
{
    return m_layoutManager->cellSize().height();
}

void AppletsLayout::setCellHeight(qreal height)
{
    if (qFuzzyCompare(height, m_layoutManager->cellSize().height())) {
        return;
    }

    m_layoutManager->setCellSize(QSizeF(m_layoutManager->cellSize().width(), height));

    emit cellHeightChanged();
}

void AppletsLayout::setAcceptsAppletCallback(const QJSValue &callback)
{
    if (m_acceptsAppletCallback.strictlyEquals(callback)) {
        return;
    }

    if (!callback.isNull() && !callback.isCallable()) {
        return;
    }

    m_acceptsAppletCallback = callback;

    Q_EMIT acceptsAppletCallbackChanged();
}

QQmlComponent *AppletsLayout::appletContainerComponent() const
{
    return m_appletContainerComponent;
}

void AppletsLayout::setAppletContainerComponent(QQmlComponent *component)
{
    if (m_appletContainerComponent == component) {
        return;
    }

    m_appletContainerComponent = component;

    emit appletContainerComponentChanged();
}

AppletsLayout::EditModeCondition AppletsLayout::editModeCondition() const
{
    return m_editModeCondition;
}

void AppletsLayout::setEditModeCondition(AppletsLayout::EditModeCondition condition)
{
    if (m_editModeCondition == condition) {
        return;
    }

    if (m_editModeCondition == Locked) {
        setEditMode(false);
    }

    m_editModeCondition = condition;

    emit editModeConditionChanged();
}

bool AppletsLayout::editMode() const
{
    return m_editMode;
}

void AppletsLayout::setEditMode(bool editMode)
{
    if (m_editMode == editMode) {
        return;
    }

    m_editMode = editMode;

    emit editModeChanged();
}

ItemContainer *AppletsLayout::placeHolder() const
{
    return m_placeHolder;
}

void AppletsLayout::setPlaceHolder(ItemContainer *placeHolder)
{
    if (m_placeHolder == placeHolder) {
        return;
    }

    m_placeHolder = placeHolder;
    m_placeHolder->setParentItem(this);
    m_placeHolder->setZ(9999);
    m_placeHolder->setOpacity(false);

    emit placeHolderChanged();
}

QQuickItem *AppletsLayout::eventManagerToFilter() const
{
    return m_eventManagerToFilter;
}

void AppletsLayout::setEventManagerToFilter(QQuickItem *item)
{
    if (m_eventManagerToFilter == item) {
        return;
    }

    m_eventManagerToFilter = item;
    setFiltersChildMouseEvents(m_eventManagerToFilter);
    emit eventManagerToFilterChanged();
}

void AppletsLayout::save()
{
    m_saveLayoutTimer->start();
}

void AppletsLayout::showPlaceHolderAt(const QRectF &geom)
{
    if (!m_placeHolder) {
        return;
    }

    m_placeHolder->setPosition(geom.topLeft());
    m_placeHolder->setSize(geom.size());

    m_layoutManager->positionItem(m_placeHolder);

    m_placeHolder->setProperty("opacity", 1);
}

void AppletsLayout::showPlaceHolderForItem(ItemContainer *item)
{
    if (!m_placeHolder) {
        return;
    }

    m_placeHolder->setPreferredLayoutDirection(item->preferredLayoutDirection());
    m_placeHolder->setPosition(item->position());
    m_placeHolder->setSize(item->size());

    m_layoutManager->positionItem(m_placeHolder);

    m_placeHolder->setProperty("opacity", 1);
}

void AppletsLayout::hidePlaceHolder()
{
    if (!m_placeHolder) {
        return;
    }

    m_placeHolder->setProperty("opacity", 0);
}

bool AppletsLayout::isRectAvailable(qreal x, qreal y, qreal width, qreal height)
{
    return m_layoutManager->isRectAvailable(QRectF(x, y, width, height));
}

bool AppletsLayout::itemIsManaged(ItemContainer *item)
{
    if (!item) {
        return false;
    }

    return m_layoutManager->itemIsManaged(item);
}

void AppletsLayout::positionItem(ItemContainer *item)
{
    if (!item) {
        return;
    }

    item->setParent(this);
    m_layoutManager->positionItemAndAssign(item);
}

void AppletsLayout::restoreItem(ItemContainer *item)
{
    m_layoutManager->restoreItem(item);
}

void AppletsLayout::releaseSpace(ItemContainer *item)
{
    if (!item) {
        return;
    }

    m_layoutManager->releaseSpace(item);
}

void AppletsLayout::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    // Ignore completely moves without resize
    if (newGeometry.size() == oldGeometry.size()) {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        return;
    }

    // Don't care for anything happening before startup completion
    if (!m_containment || !m_containment->corona() || !m_containment->corona()->isStartupCompleted()) {
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
        return;
    }

    // Only do a layouting procedure if we received a valid size
    if (!newGeometry.isEmpty()) {
        m_sizeSyncTimer->start();
    }

    QQuickItem::geometryChanged(newGeometry, oldGeometry);
}

void AppletsLayout::updatePolish()
{
    m_layoutManager->resetLayout();
    m_savedSize = size();
}

void AppletsLayout::componentComplete()
{
    if (!m_containment || !m_containmentItem) {
        QQuickItem::componentComplete();
        return;
    }

    if (!m_configKey.isEmpty()) {
        m_layoutManager->parseLayout(m_containment->config().readEntry(m_configKey, ""));
    }

    const QList<QObject *> appletObjects = m_containmentItem->property("applets").value<QList<QObject *>>();

    for (auto *obj : appletObjects) {
        PlasmaQuick::AppletQuickItem *appletItem = qobject_cast<PlasmaQuick::AppletQuickItem *>(obj);

        if (!obj) {
            continue;
        }

        AppletContainer *container = createContainerForApplet(appletItem);
        if (width() > 0 && height() > 0) {
            m_layoutManager->positionItemAndAssign(container);
        }
    }

    // layout all extra non applet items
    if (width() > 0 && height() > 0) {
        for (auto *child : childItems()) {
            ItemContainer *item = qobject_cast<ItemContainer *>(child);
            if (item && item != m_placeHolder && !m_layoutManager->itemIsManaged(item)) {
                m_layoutManager->positionItemAndAssign(item);
            }
        }
    }

    if (m_containment && m_containment->corona()) {
        connect(m_containment->corona(), &Plasma::Corona::startupCompleted, this, []() {
            // m_savedSize = size();
        });
        // When the screen geometry changes, we need to know the geometry just before it did, so we can apply out heuristic of keeping the distance with borders
        // constant
        connect(m_containment->corona(), &Plasma::Corona::screenGeometryChanged, this, [this](int id) {
            if (m_containment->screen() == id) {
                m_geometryBeforeResolutionChange = QRectF(x(), y(), width(), height());
            }
        });
    }
    QQuickItem::componentComplete();
}

bool AppletsLayout::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    if (item != m_eventManagerToFilter) {
        return QQuickItem::childMouseEventFilter(item, event);
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (me->buttons() & Qt::LeftButton) {
            mousePressEvent(me);
        }
        break;
    }
    case QEvent::MouseMove: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        mouseMoveEvent(me);
        break;
    }
    case QEvent::MouseButtonRelease: {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        mouseReleaseEvent(me);
        break;
    }
    case QEvent::UngrabMouse:
        mouseUngrabEvent();
        break;
    default:
        break;
    }

    return QQuickItem::childMouseEventFilter(item, event);
}

void AppletsLayout::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus(Qt::MouseFocusReason);

    if (!m_editMode && m_editModeCondition == AppletsLayout::Manual) {
        return;
    }

    if (!m_editMode && m_editModeCondition == AppletsLayout::AfterPressAndHold) {
        m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }

    m_mouseDownWasEditMode = m_editMode;
    m_mouseDownPosition = event->windowPos();

    // event->setAccepted(false);
}

void AppletsLayout::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_editMode && m_editModeCondition == AppletsLayout::Manual) {
        return;
    }

    if (!m_editMode && QPointF(event->windowPos() - m_mouseDownPosition).manhattanLength() >= QGuiApplication::styleHints()->startDragDistance()) {
        m_pressAndHoldTimer->stop();
    }
}

void AppletsLayout::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_editMode
        && m_mouseDownWasEditMode
        // By only accepting synthetyzed events, this makes the
        // close by tapping in any empty area only work with real
        // touch events, as we want a different behavior between desktop
        // and tablet mode
        && (event->source() == Qt::MouseEventSynthesizedBySystem || event->source() == Qt::MouseEventSynthesizedByQt)
        && QPointF(event->windowPos() - m_mouseDownPosition).manhattanLength() < QGuiApplication::styleHints()->startDragDistance()) {
        setEditMode(false);
    }

    m_pressAndHoldTimer->stop();

    if (!m_editMode) {
        for (auto *child : childItems()) {
            ItemContainer *item = qobject_cast<ItemContainer *>(child);
            if (item && item != m_placeHolder) {
                item->setEditMode(false);
            }
        }
    }
}

void AppletsLayout::mouseUngrabEvent()
{
    m_pressAndHoldTimer->stop();
}

void AppletsLayout::appletAdded(QObject *applet, int x, int y)
{
    PlasmaQuick::AppletQuickItem *appletItem = qobject_cast<PlasmaQuick::AppletQuickItem *>(applet);

    // maybe even an assert?
    if (!appletItem) {
        return;
    }

    if (m_acceptsAppletCallback.isCallable()) {
        QQmlEngine *engine = QQmlEngine::contextForObject(this)->engine();
        Q_ASSERT(engine);
        QJSValueList args;
        args << engine->newQObject(applet) << QJSValue(x) << QJSValue(y);

        if (!m_acceptsAppletCallback.call(args).toBool()) {
            emit appletRefused(applet, x, y);
            return;
        }
    }

    AppletContainer *container = createContainerForApplet(appletItem);
    container->setPosition(QPointF(x, y));
    container->setVisible(true);

    m_layoutManager->positionItemAndAssign(container);
}

void AppletsLayout::appletRemoved(QObject *applet)
{
    PlasmaQuick::AppletQuickItem *appletItem = qobject_cast<PlasmaQuick::AppletQuickItem *>(applet);

    // maybe even an assert?
    if (!appletItem) {
        return;
    }

    AppletContainer *container = m_containerForApplet.value(appletItem);
    if (!container) {
        return;
    }

    m_layoutManager->releaseSpace(container);
    m_containerForApplet.remove(appletItem);
    appletItem->setParentItem(this);
    container->deleteLater();
}

AppletContainer *AppletsLayout::createContainerForApplet(PlasmaQuick::AppletQuickItem *appletItem)
{
    AppletContainer *container = m_containerForApplet.value(appletItem);

    if (container) {
        return container;
    }

    bool createdFromQml = true;

    if (m_appletContainerComponent) {
        QQmlContext *context = QQmlEngine::contextForObject(this);
        Q_ASSERT(context);
        QObject *instance = m_appletContainerComponent->beginCreate(context);
        container = qobject_cast<AppletContainer *>(instance);
        if (container) {
            container->setParentItem(this);
        } else {
            qWarning() << "Error: provided component not an AppletContainer instance";
            if (instance) {
                instance->deleteLater();
            }
            createdFromQml = false;
        }
    }

    if (!container) {
        container = new AppletContainer(this);
    }

    container->setVisible(false);

    const QSizeF appletSize = appletItem->size();
    container->setContentItem(appletItem);

    m_containerForApplet[appletItem] = container;
    container->setLayout(this);
    container->setKey(QLatin1String("Applet-") + QString::number(appletItem->applet()->id()));

    const bool geometryWasSaved = m_layoutManager->restoreItem(container);

    if (!geometryWasSaved) {
        container->setPosition(QPointF(appletItem->x() - container->leftPadding(), appletItem->y() - container->topPadding()));

        if (!appletSize.isEmpty()) {
            container->setSize(QSizeF(qMax(m_minimumItemSize.width(), appletSize.width() + container->leftPadding() + container->rightPadding()),
                                      qMax(m_minimumItemSize.height(), appletSize.height() + container->topPadding() + container->bottomPadding())));
        }
    }

    if (m_appletContainerComponent && createdFromQml) {
        m_appletContainerComponent->completeCreate();
    }

    // NOTE: This has to be done here as we need the component completed to have all the bindings evaluated
    if (!geometryWasSaved && appletSize.isEmpty()) {
        if (container->initialSize().width() > m_minimumItemSize.width() && container->initialSize().height() > m_minimumItemSize.height()) {
            const QSizeF size = m_layoutManager->cellAlignedContainingSize(container->initialSize());
            container->setSize(size);
        } else {
            container->setSize(
                QSizeF(qMax(m_minimumItemSize.width(), m_defaultItemSize.width()), qMax(m_minimumItemSize.height(), m_defaultItemSize.height())));
        }
    }

    container->setVisible(true);
    appletItem->setVisible(true);

    return container;
}

#include "moc_appletslayout.cpp"
