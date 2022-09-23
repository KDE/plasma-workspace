/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QPointer>
#include <QQmlParserStatus>
#include <QQuickItem>
#include <QQuickWindow>

class QTimer;

namespace Plasma
{
class Containment;
}

namespace PlasmaQuick
{
class AppletQuickItem;
}

class AbstractLayoutManager;
class AppletContainer;
class ItemContainer;

class AppletsLayout : public QQuickItem
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(QString configKey READ configKey WRITE setConfigKey NOTIFY configKeyChanged)

    // A config key that can be used as fallback when loading and configKey is not found
    // Is always a backup of the last used configKey. Useful when the configkey depends
    // from the screen size and plasma starts on an "unexpected" size
    Q_PROPERTY(QString fallbackConfigKey READ fallbackConfigKey WRITE setFallbackConfigKey NOTIFY fallbackConfigKeyChanged)

    Q_PROPERTY(bool relayoutLock READ relayoutLock WRITE setRelayoutLock NOTIFY relayoutLockChanged)

    Q_PROPERTY(PlasmaQuick::AppletQuickItem *containment READ containment WRITE setContainment NOTIFY containmentChanged)

    Q_PROPERTY(QJSValue acceptsAppletCallback READ acceptsAppletCallback WRITE setAcceptsAppletCallback NOTIFY acceptsAppletCallbackChanged)

    Q_PROPERTY(qreal minimumItemWidth READ minimumItemWidth WRITE setMinimumItemWidth NOTIFY minimumItemWidthChanged)

    Q_PROPERTY(qreal minimumItemHeight READ minimumItemHeight WRITE setMinimumItemHeight NOTIFY minimumItemHeightChanged)

    Q_PROPERTY(qreal defaultItemWidth READ defaultItemWidth WRITE setDefaultItemWidth NOTIFY defaultItemWidthChanged)

    Q_PROPERTY(qreal defaultItemHeight READ defaultItemHeight WRITE setDefaultItemHeight NOTIFY defaultItemHeightChanged)

    Q_PROPERTY(qreal cellWidth READ cellWidth WRITE setCellWidth NOTIFY cellWidthChanged)

    Q_PROPERTY(qreal cellHeight READ cellHeight WRITE setCellHeight NOTIFY cellHeightChanged)

    Q_PROPERTY(QQmlComponent *appletContainerComponent READ appletContainerComponent WRITE setAppletContainerComponent NOTIFY appletContainerComponentChanged)

    Q_PROPERTY(ItemContainer *placeHolder READ placeHolder WRITE setPlaceHolder NOTIFY placeHolderChanged);

    /**
     * if the applets layout contains some kind of main MouseArea,
     * MouseEventListener or Flickable, we want to filter its events to make the
     * long mouse press work
     */
    Q_PROPERTY(QQuickItem *eventManagerToFilter READ eventManagerToFilter WRITE setEventManagerToFilter NOTIFY eventManagerToFilterChanged);

    Q_PROPERTY(AppletsLayout::EditModeCondition editModeCondition READ editModeCondition WRITE setEditModeCondition NOTIFY editModeConditionChanged)
    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)

public:
    enum PreferredLayoutDirection {
        Closest = 0,
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop,
    };
    Q_ENUM(PreferredLayoutDirection)

    enum EditModeCondition {
        Locked = 0,
        Manual,
        AfterPressAndHold,
    };
    Q_ENUM(EditModeCondition)

    enum LayoutChange {
        NoChange = 0,
        SizeChange = 1,
        ConfigKeyChange = 2,
    };
    Q_DECLARE_FLAGS(LayoutChanges, LayoutChange)

    AppletsLayout(QQuickItem *parent = nullptr);
    ~AppletsLayout();

    // QML setters and getters
    QString configKey() const;
    void setConfigKey(const QString &key);

    QString fallbackConfigKey() const;
    void setFallbackConfigKey(const QString &key);

    bool relayoutLock() const;
    void setRelayoutLock(bool lock);

    PlasmaQuick::AppletQuickItem *containment() const;
    void setContainment(PlasmaQuick::AppletQuickItem *containment);

    QJSValue acceptsAppletCallback() const;
    void setAcceptsAppletCallback(const QJSValue &callback);

    qreal minimumItemWidth() const;
    void setMinimumItemWidth(qreal width);

    qreal minimumItemHeight() const;
    void setMinimumItemHeight(qreal height);

    qreal defaultItemWidth() const;
    void setDefaultItemWidth(qreal width);

    qreal defaultItemHeight() const;
    void setDefaultItemHeight(qreal height);

    qreal cellWidth() const;
    void setCellWidth(qreal width);

    qreal cellHeight() const;
    void setCellHeight(qreal height);

    QQmlComponent *appletContainerComponent() const;
    void setAppletContainerComponent(QQmlComponent *component);

    ItemContainer *placeHolder() const;
    void setPlaceHolder(ItemContainer *placeHolder);

    QQuickItem *eventManagerToFilter() const;
    void setEventManagerToFilter(QQuickItem *item);

    EditModeCondition editModeCondition() const;
    void setEditModeCondition(EditModeCondition condition);

    bool editMode() const;
    void setEditMode(bool edit);

    Q_INVOKABLE void save();
    Q_INVOKABLE void showPlaceHolderAt(const QRectF &geom);
    Q_INVOKABLE void showPlaceHolderForItem(ItemContainer *item);
    Q_INVOKABLE void hidePlaceHolder();

    Q_INVOKABLE bool isRectAvailable(qreal x, qreal y, qreal width, qreal height);
    Q_INVOKABLE bool itemIsManaged(ItemContainer *item);
    Q_INVOKABLE void positionItem(ItemContainer *item);
    Q_INVOKABLE void restoreItem(ItemContainer *item);
    Q_INVOKABLE void releaseSpace(ItemContainer *item);

Q_SIGNALS:
    /**
     * An applet has been refused by the layout: acceptsAppletCallback
     * returned false and will need to be managed in a different way
     */
    void appletRefused(QObject *applet, int x, int y);

    void configKeyChanged();
    void fallbackConfigKeyChanged();
    void relayoutLockChanged();
    void containmentChanged();
    void minimumItemWidthChanged();
    void minimumItemHeightChanged();
    void defaultItemWidthChanged();
    void defaultItemHeightChanged();
    void cellWidthChanged();
    void cellHeightChanged();
    void acceptsAppletCallbackChanged();
    void appletContainerComponentChanged();
    void placeHolderChanged();
    void eventManagerToFilterChanged();
    void editModeConditionChanged();
    void editModeChanged();

protected:
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    void updatePolish() override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

    // void classBegin() override;
    void componentComplete() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;

private Q_SLOTS:
    void appletAdded(QObject *applet, int x, int y);
    void appletRemoved(QObject *applet);

private:
    AppletContainer *createContainerForApplet(PlasmaQuick::AppletQuickItem *appletItem);

    QString m_configKey;
    QString m_fallbackConfigKey;
    QTimer *m_saveLayoutTimer;
    QTimer *m_layoutChangeTimer;
    LayoutChanges m_layoutChanges = NoChange;

    PlasmaQuick::AppletQuickItem *m_containmentItem = nullptr;
    Plasma::Containment *m_containment = nullptr;
    QQmlComponent *m_appletContainerComponent = nullptr;

    AbstractLayoutManager *m_layoutManager = nullptr;

    QPointer<ItemContainer> m_placeHolder;
    QPointer<QQuickItem> m_eventManagerToFilter;

    QTimer *m_pressAndHoldTimer;

    QJSValue m_acceptsAppletCallback;

    AppletsLayout::EditModeCondition m_editModeCondition = AppletsLayout::Manual;

    QHash<PlasmaQuick::AppletQuickItem *, AppletContainer *> m_containerForApplet;

    QSizeF m_minimumItemSize;
    QSizeF m_defaultItemSize;
    QSizeF m_savedSize;
    QRectF m_geometryBeforeResolutionChange;

    QPointF m_mouseDownPosition = QPoint(-1, -1);
    bool m_mouseDownWasEditMode = false;
    bool m_editMode = false;
    bool m_relayoutLock = false;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(AppletsLayout::LayoutChanges)
