/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "appletslayout.h"
#include "appletcontainer.h"
#include "containmentlayoutmanager_debug.h"
#include "gridlayoutmanager.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QStyleHints>
#include <QTimer>

// Plasma
#include <Containment>
#include <Corona>
#include <PlasmaQuick/AppletQuickItem>
#include <chrono>
#include <plasma/containment.h>

using namespace std::chrono_literals;

AppletsLayout::AppletsLayout(QQuickItem *parent)
    : QQuickItem(parent)
{
    m_layoutManager = new GridLayoutManager(this);

    setFlags(QQuickItem::ItemIsFocusScope);
    setAcceptedMouseButtons(Qt::LeftButton);

    m_saveLayoutTimer = new QTimer(this);
    m_saveLayoutTimer->setSingleShot(true);
    m_saveLayoutTimer->setInterval(100ms);
    connect(m_layoutManager, &AbstractLayoutManager::layoutNeedsSaving, m_saveLayoutTimer, QOverload<>::of(&QTimer::start));
    connect(m_saveLayoutTimer, &QTimer::timeout, this, [this]() {
        // We can't assume m_containment to be valid: if we load in a plasmoid that can run also
        // in "applet" mode, m_containment will never be valid
        if (!m_containment) {
            return;
        }
        // We can't save the layout during bootup, for performance reasons and to avoid race consitions as much as possible, so if we needto save and still
        // starting up, don't actually savenow, but we will when Corona::startupCompleted is emitted

        if (!m_configKey.isEmpty() && m_containment && m_containment->corona()->isStartupCompleted()) {
            const QString serializedConfig = m_layoutManager->serializeLayout();
            m_containment->config().writeEntry(m_configKey, serializedConfig);
            m_containment->config().writeEntry(m_fallbackConfigKey, serializedConfig);
            // FIXME: something more efficient
            m_layoutManager->parseLayout(serializedConfig);
            m_savedSize = size();
            m_containment->corona()->requireConfigSync();
        }
    });

    m_layoutChangeTimer = new QTimer(this);
    m_layoutChangeTimer->setSingleShot(true);
    m_layoutChangeTimer->setInterval(100ms);
    connect(m_layoutChangeTimer, &QTimer::timeout, this, [this]() {
        // We can't assume m_containment to be valid: if we load in a plasmoid that can run also
        // in "applet" mode, m_containment will never be valid
        if (!m_containment || width() <= 0 || height() <= 0 || m_relayoutLock) {
            return;
        }

        const QString &serializedConfig = m_containment->config().readEntry(m_configKey, "");
        if ((m_layoutChanges & ConfigKeyChange)) {
            if (!m_configKey.isEmpty() && m_containment) {
                m_layoutManager->parseLayout(serializedConfig);
                if (width() > 0 && height() > 0) {
                    if (m_geometryBeforeResolutionChange.isEmpty()) {
                        m_layoutManager->resetLayoutFromConfig(QRectF(), QRectF());
                    } else {
                        m_layoutManager->resetLayoutFromConfig(QRectF(x(), y(), width(), height()), m_geometryBeforeResolutionChange);
                    }
                    m_savedSize = size();
                }
            }
        } else if (m_layoutChanges & SizeChange) {
            const QRect newGeom(x(), y(), width(), height());
            // The size has been restored from the last one it has been saved: restore that exact same layout
            if (newGeom.size() == m_savedSize) {
                m_layoutManager->resetLayoutFromConfig(QRectF(), QRectF());

                // If the resize is consequence of a screen resolution change, queue a relayout maintaining the distance between screen edges
            } else if (!m_geometryBeforeResolutionChange.isEmpty()) {
                m_layoutManager->layoutGeometryChanged(newGeom, m_geometryBeforeResolutionChange);
                m_geometryBeforeResolutionChange = QRectF();

                // If the user doesn't move a widget after this is done, the widget positions won't be saved and they will be in the wrong
                // places on next login, so save them now.

                save();
            }
        }
        m_layoutChanges = NoChange;
    });
    m_pressAndHoldTimer = new QTimer(this);
    m_pressAndHoldTimer->setSingleShot(true);
    connect(m_pressAndHoldTimer, &QTimer::timeout, this, [this]() {
        setEditMode(true);
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
        qCWarning(CONTAINMENTLAYOUTMANAGER_DEBUG) << "Error: cannot change the containment to AppletsLayout";
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

    Q_EMIT containmentChanged();
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
    m_layoutChanges |= ConfigKeyChange;
    m_layoutChangeTimer->start();

    Q_EMIT configKeyChanged();
}

QString AppletsLayout::fallbackConfigKey() const
{
    return m_fallbackConfigKey;
}

void AppletsLayout::setFallbackConfigKey(const QString &key)
{
    if (m_fallbackConfigKey == key) {
        return;
    }

    m_fallbackConfigKey = key;

    Q_EMIT fallbackConfigKeyChanged();
}

bool AppletsLayout::relayoutLock() const
{
    return m_relayoutLock;
}

void AppletsLayout::setRelayoutLock(bool lock)
{
    if (lock == m_relayoutLock) {
        return;
    }

    m_relayoutLock = lock;

    if (!lock && m_layoutChanges != NoChange) {
        m_layoutChangeTimer->start();
    }

    Q_EMIT relayoutLockChanged();
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

    Q_EMIT minimumItemWidthChanged();
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

    Q_EMIT minimumItemHeightChanged();
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

    Q_EMIT defaultItemWidthChanged();
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

    Q_EMIT defaultItemHeightChanged();
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

    Q_EMIT cellWidthChanged();
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

    Q_EMIT cellHeightChanged();
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

    Q_EMIT appletContainerComponentChanged();
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

    Q_EMIT editModeConditionChanged();
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

    Q_EMIT editModeChanged();
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

    Q_EMIT placeHolderChanged();
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
    Q_EMIT eventManagerToFilterChanged();
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void AppletsLayout::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
#else
void AppletsLayout::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
#endif
{
    // Ignore completely moves without resize
    if (newGeometry.size() == oldGeometry.size()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
#else
        QQuickItem::geometryChange(newGeometry, oldGeometry);
#endif
        return;
    }

    // Don't care for anything happening before startup completion
    if (!m_containment || !m_containment->corona() || !m_containment->corona()->isStartupCompleted()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QQuickItem::geometryChanged(newGeometry, oldGeometry);
#else
        QQuickItem::geometryChange(newGeometry, oldGeometry);
#endif
        return;
    }

    // Only do a layouting procedure if we received a valid size
    if (!newGeometry.isEmpty() && newGeometry != oldGeometry) {
        m_layoutChanges |= SizeChange;
        m_layoutChangeTimer->start();
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QQuickItem::geometryChanged(newGeometry, oldGeometry);
#else
    QQuickItem::geometryChange(newGeometry, oldGeometry);
#endif
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
        const QString &serializedConfig = m_containment->config().readEntry(m_configKey, "");
        if (!serializedConfig.isEmpty()) {
            m_layoutManager->parseLayout(serializedConfig);
        } else {
            m_layoutManager->parseLayout(m_containment->config().readEntry(m_fallbackConfigKey, ""));
        }
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
        // We inhibit save during startup, so actually save now that startup is completed
        connect(m_containment->corona(), &Plasma::Corona::startupCompleted, this, [this]() {
            save();
        });
        // When the screen geometry changes, we need to know the geometry just before it did, so we can apply out heuristic of keeping the distance with borders
        // constant
        connect(m_containment->corona(), &Plasma::Corona::screenGeometryChanged, this, [this](int id) {
            if (m_containment->screen() == id) {
                m_geometryBeforeResolutionChange = QRectF(x(), y(), width(), height());
                m_layoutChangeTimer->start();
            }
        });
        // If the containment is moved to a different screen, treat it like a resolution change
        connect(m_containment, &Plasma::Containment::screenChanged, this, [this]() {
            m_geometryBeforeResolutionChange = QRectF(x(), y(), width(), height());
            m_layoutChangeTimer->start();
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

    // Only accept synthesized events i.e. touch events, because we only want
    // to support press-and-hold. Click-and-hold is weird. See 457979.
    if (!(event->source() == Qt::MouseEventSynthesizedBySystem || event->source() == Qt::MouseEventSynthesizedByQt)) {
        const auto children = childItems();
        // If any container is in edit mode, accept the press event so we can
        // cancel the edit mode. If not, don't accept the event so it can be
        // passed on to other parts.
        if (std::none_of(children.begin(), children.end(), [](QQuickItem *child) {
                auto container = qobject_cast<ItemContainer *>(child);
                return container ? container->editMode() : false;
            })) {
            event->setAccepted(false);
        }
        return;
    }

    if (!m_editMode && m_editModeCondition == AppletsLayout::Manual) {
        return;
    }

    if (!m_editMode && m_editModeCondition == AppletsLayout::AfterPressAndHold) {
        m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    }

    m_mouseDownWasEditMode = m_editMode;
    m_mouseDownPosition = event->windowPos();
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
            Q_EMIT appletRefused(applet, x, y);
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
            qCWarning(CONTAINMENTLAYOUTMANAGER_DEBUG) << "Error: provided component not an AppletContainer instance";
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
