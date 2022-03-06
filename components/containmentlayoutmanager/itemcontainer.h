/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QPointer>
#include <QQmlParserStatus>
#include <QQuickItem>

#include "appletslayout.h"

class QTimer;

class ConfigOverlay;

class ItemContainer : public QQuickItem
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(AppletsLayout *layout READ layout NOTIFY layoutChanged)
    // TODO: make it unchangeable? probably not
    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
    Q_PROPERTY(ItemContainer::EditModeCondition editModeCondition READ editModeCondition WRITE setEditModeCondition NOTIFY editModeConditionChanged)
    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)
    Q_PROPERTY(bool dragActive READ dragActive NOTIFY dragActiveChanged)
    Q_PROPERTY(AppletsLayout::PreferredLayoutDirection preferredLayoutDirection READ preferredLayoutDirection WRITE setPreferredLayoutDirection NOTIFY
                   preferredLayoutDirectionChanged)

    Q_PROPERTY(QQmlComponent *configOverlayComponent READ configOverlayComponent WRITE setConfigOverlayComponent NOTIFY configOverlayComponentChanged)
    Q_PROPERTY(bool configOverlayVisible READ configOverlayVisible WRITE setConfigOverlayVisible NOTIFY configOverlayVisibleChanged)
    Q_PROPERTY(QQuickItem *configOverlayItem READ configOverlayItem NOTIFY configOverlayItemChanged)

    /**
     * Initial size this container asks to have upon creation. only positive values are considered
     */
    Q_PROPERTY(QSizeF initialSize READ initialSize WRITE setInitialSize NOTIFY initialSizeChanged)
    // From there mostly a clone of QQC2 Control
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged)
    Q_PROPERTY(QQuickItem *background READ background WRITE setBackground NOTIFY backgroundChanged)

    /**
     * Padding adds a space between each edge of the content item and the background item, effectively controlling the size of the content item.
     */
    Q_PROPERTY(int leftPadding READ leftPadding WRITE setLeftPadding NOTIFY leftPaddingChanged)
    Q_PROPERTY(int rightPadding READ rightPadding WRITE setRightPadding NOTIFY rightPaddingChanged)
    Q_PROPERTY(int topPadding READ topPadding WRITE setTopPadding NOTIFY topPaddingChanged)
    Q_PROPERTY(int bottomPadding READ bottomPadding WRITE setBottomPadding NOTIFY bottomPaddingChanged)

    /**
     * The size of the contents: the size of this item minus the padding
     */
    Q_PROPERTY(int contentWidth READ contentWidth NOTIFY contentWidthChanged)
    Q_PROPERTY(int contentHeight READ contentHeight NOTIFY contentHeightChanged)

    Q_PROPERTY(QQmlListProperty<QObject> contentData READ contentData FINAL)
    // Q_CLASSINFO("DeferredPropertyNames", "background,contentItem")
    Q_CLASSINFO("DefaultProperty", "contentData")

public:
    enum EditModeCondition {
        Locked = AppletsLayout::EditModeCondition::Locked,
        Manual = AppletsLayout::EditModeCondition::Manual,
        AfterPressAndHold = AppletsLayout::EditModeCondition::AfterPressAndHold,
        AfterPress,
        AfterMouseOver,
    };
    Q_ENUMS(EditModeCondition)

    ItemContainer(QQuickItem *parent = nullptr);
    ~ItemContainer();

    QQmlListProperty<QObject> contentData();

    QString key() const;
    void setKey(const QString &id);

    bool editMode() const;
    void setEditMode(bool edit);

    bool dragActive() const;

    Q_INVOKABLE void cancelEdit();

    EditModeCondition editModeCondition() const;
    void setEditModeCondition(EditModeCondition condition);

    AppletsLayout::PreferredLayoutDirection preferredLayoutDirection() const;
    void setPreferredLayoutDirection(AppletsLayout::PreferredLayoutDirection direction);

    QQmlComponent *configOverlayComponent() const;
    void setConfigOverlayComponent(QQmlComponent *component);

    bool configOverlayVisible() const;
    void setConfigOverlayVisible(bool visible);

    // TODO: keep this accessible?
    ConfigOverlay *configOverlayItem() const;

    QSizeF initialSize() const;
    void setInitialSize(const QSizeF &size);

    // Control-like api
    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

    QQuickItem *background() const;
    void setBackground(QQuickItem *item);

    // Setters and getters for the padding
    int leftPadding() const;
    void setLeftPadding(int padding);

    int topPadding() const;
    void setTopPadding(int padding);

    int rightPadding() const;
    void setRightPadding(int padding);

    int bottomPadding() const;
    void setBottomPadding(int padding);

    int contentWidth() const;
    int contentHeight() const;

    AppletsLayout *layout() const;

    // Not for QML
    void setLayout(AppletsLayout *layout);

    QObject *layoutAttached() const
    {
        return m_layoutAttached;
    }

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

    // void classBegin() override;
    void componentComplete() override;
    bool childMouseEventFilter(QQuickItem *item, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseUngrabEvent() override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

Q_SIGNALS:

    /**
     * The user manually dragged the ItemContainer around
     * @param newPosition new position of the ItemContainer in parent coordinates
     * @param dragCenter position in ItemContainer coordinates of the drag hotspot, i.e. where the user pressed the mouse or the
     * finger over the ItemContainer
     */
    void userDrag(const QPointF &newPosition, const QPointF &dragCenter);

    void dragActiveChanged();

    /**
     * The attached layout object changed some of its size hints
     */
    void sizeHintsChanged();

    // QML property notifiers
    void layoutChanged();
    void keyChanged();
    void editModeConditionChanged();
    void editModeChanged(bool editMode);
    void preferredLayoutDirectionChanged();
    void configOverlayComponentChanged();
    void configOverlayItemChanged();
    void initialSizeChanged();
    void configOverlayVisibleChanged(bool configOverlayVisile);

    void backgroundChanged();
    void contentItemChanged();
    void leftPaddingChanged();
    void rightPaddingChanged();
    void topPaddingChanged();
    void bottomPaddingChanged();
    void contentWidthChanged();
    void contentHeightChanged();

private:
    void syncChildItemsGeometry(const QSizeF &size);
    void sendUngrabRecursive(QQuickItem *item);

    // internal accessorts for the contentData QProperty
    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *object);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    static int contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, int index);
#else
    static qsizetype contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, qsizetype index);
#endif
    static void contentData_clear(QQmlListProperty<QObject> *prop);

    QPointer<QQuickItem> m_contentItem;
    QPointer<QQuickItem> m_backgroundItem;

    // Internal implementation detail: this is used to reparent all items to contentItem
    QList<QObject *> m_contentData;

    /**
     * Padding adds a space between each edge of the content item and the background item, effectively controlling the size of the content item.
     */
    int m_leftPadding = 0;
    int m_rightPadding = 0;
    int m_topPadding = 0;
    int m_bottomPadding = 0;

    QString m_key;

    QPointer<AppletsLayout> m_layout;
    QTimer *m_editModeTimer = nullptr;
    QTimer *m_closeEditModeTimer = nullptr;
    QTimer *m_sizeHintAdjustTimer = nullptr;
    QObject *m_layoutAttached = nullptr;
    EditModeCondition m_editModeCondition = Manual;
    QSizeF m_initialSize;

    QPointer<QQmlComponent> m_configOverlayComponent;
    ConfigOverlay *m_configOverlay = nullptr;

    QPointF m_lastMousePosition = QPoint(-1, -1);
    QPointF m_mouseDownPosition = QPoint(-1, -1);
    AppletsLayout::PreferredLayoutDirection m_preferredLayoutDirection = AppletsLayout::Closest;
    bool m_editMode = false;
    bool m_mouseDown = false;
    bool m_mouseSynthetizedFromTouch = false;
    bool m_dragActive = false;
};
