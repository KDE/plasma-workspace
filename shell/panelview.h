/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Plasma/Theme>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#ifdef HAVE_X11
#include <QWindow> // For WId
#endif

#include <KSvg/FrameSvg>

#include <PlasmaQuick/ConfigView>
#include <PlasmaQuick/ContainmentView>
#include <PlasmaQuick/PopupPlasmaWindow>

class AutoHideScreenEdge;
class ShellCorona;
class PanelConfigView;

namespace LayerShellQt
{
class Window;
}

class PanelView : public PlasmaQuick::ContainmentView

{
    Q_OBJECT
    /**
     * Alignment of the panel: when not fullsize it can be aligned at left,
     * right or center of the screen (left and right work as top/bottom
     * too for vertical panels)
     */
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)

    /**
     * how much the panel is moved from the left/right/center anchor point
     */
    Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)

    /**
     * Height of horizontal panels or width of vertical panels, set by the user.
     */
    Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)

    /**
     * Preferred (natural) width of horizontal panels or height of vertical
     * panels, given its current thickness and content.  When the panel is
     * constrained by other factors such as minimumLength, maximumLength or
     * screen size, its reported length may differ from an actual width or
     * height.
     */
    Q_PROPERTY(int length READ length WRITE setLength NOTIFY lengthChanged)

    /**
     * if the panel resizes itself, never resize more than that
     */
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength NOTIFY maximumLengthChanged)

    /**
     * if the panel resizes itself, never resize less than that
     */
    Q_PROPERTY(int minimumLength READ minimumLength WRITE setMinimumLength NOTIFY minimumLengthChanged)

    /**
     * how much the panel is distant for the screen edge: used by the panel controller to drag it around
     */
    Q_PROPERTY(int distance READ distance WRITE setDistance NOTIFY distanceChanged)

    /**
     * support NoBackground in order to disable blur/contrast effects and remove
     * the panel shadows
     * @since 5.9
     */
    Q_PROPERTY(Plasma::Types::BackgroundHints backgroundHints WRITE setBackgroundHints READ backgroundHints NOTIFY backgroundHintsChanged)

    /**
     * The borders that should have a shadow
     * @since 5.7
     */
    Q_PROPERTY(KSvg::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)

    /**
     * information about the screen in which the panel is in
     */
    Q_PROPERTY(QScreen *screenToFollow READ screenToFollow WRITE setScreenToFollow NOTIFY screenToFollowChanged)

    /**
     *  how the panel behaves, visible, autohide etc.
     */
    Q_PROPERTY(VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)

    /**
     *  Property that determines how a panel's opacity behaves.
     *
     * @see OpacityMode
     */
    Q_PROPERTY(OpacityMode opacityMode READ opacityMode WRITE setOpacityMode NOTIFY opacityModeChanged)

    /**
     *  Property that determines how a panel's length behaves.
     *
     * @see LengthMode
     */
    Q_PROPERTY(LengthMode lengthMode READ lengthMode WRITE setLengthMode NOTIFY lengthModeChanged)

    /**
     *  Property that determines whether adaptive opacity is used.
     */
    Q_PROPERTY(bool adaptiveOpacityEnabled READ adaptiveOpacityEnabled NOTIFY adaptiveOpacityEnabledChanged)

    /**
     * Property that determines whether the panel is currently floating or not
     * @since 5.25
     */
    Q_PROPERTY(bool floating READ floating WRITE setFloating NOTIFY floatingChanged)

    /**
     * The minimum thickness in pixels that the panel can have.
     * @since 5.27
     */
    Q_PROPERTY(int minThickness READ minThickness NOTIFY minThicknessChanged)

public:
    enum VisibilityMode {
        NormalPanel = 0, /** default, always visible panel, the windowmanager reserves a places for it */
        AutoHide, /**the panel will be shownn only if the mouse cursor is on screen edges */
        DodgeWindows, /* the panel will be normally visible, but will hide if a window would cover it */
        WindowsGoBelow, /* the panel is always visible but will not reserve any space */
    };
    Q_ENUM(VisibilityMode)

    /** Enumeration of possible opacity modes. */
    enum OpacityMode {
        Adaptive = 0, /** The panel will change opacity depending on the presence of a maximized window */
        Opaque, /** The panel will always be opaque */
        Translucent /** The panel will always be translucent */
    };
    Q_ENUM(OpacityMode)

    /** Enumeration of possible length sizing modes. */
    enum LengthMode {
        FillAvailable = 0, /** The panel will always fill all the available screen width/height */
        FitContent, /** The panel will always have the same size as its content */
        Custom /** The panel size and offset can be set by the user */
    };
    Q_ENUM(LengthMode)

    explicit PanelView(ShellCorona *corona, QScreen *targetScreen = nullptr, QWindow *parent = nullptr);
    ~PanelView() override;

    KConfigGroup config() const override;
    KConfigGroup configDefaults() const;

    Q_INVOKABLE QString fileFromPackage(const QString &key, const QString &fileName);
    Q_INVOKABLE void maximize();

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    int offset() const;
    void setOffset(int offset);

    int thickness() const;
    void setThickness(int thickness);
    int totalThickness() const;

    int length() const;
    void setLength(int value);

    int maximumLength() const;
    void setMaximumLength(int length);

    int minimumLength() const;
    void setMinimumLength(int length);

    int distance() const;
    void setDistance(int dist);

    bool floating() const;
    void setFloating(bool floating);

    int minThickness() const;

    QRegion configRegion() const;

    Plasma::Types::BackgroundHints backgroundHints() const;
    void setBackgroundHints(Plasma::Types::BackgroundHints hint);

    KSvg::FrameSvg::EnabledBorders enabledBorders() const;

    VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    PanelView::OpacityMode opacityMode() const;
    bool adaptiveOpacityEnabled();
    void setOpacityMode(PanelView::OpacityMode mode);
    PanelView::LengthMode lengthMode() const;
    void setLengthMode(PanelView::LengthMode mode);
    void updateAdaptiveOpacityEnabled();

    /**
     * @returns the geometry of the panel given a distance
     */
    Q_INVOKABLE QRect geometryByDistance(int distance) const;

    /**
     * @returns the dodge/defloat geometry of the panel given a distance
     */
    Q_INVOKABLE QRect dogdeGeometryByDistance(int distance) const;

    /* Both Shared with script/panel.cpp */
    static KConfigGroup panelConfig(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen);
    static KConfigGroup panelConfigDefaults(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen);

    void updateExclusiveZone();

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen *screen);
    QScreen *screenToFollow() const;

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *ev) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *e) override;

Q_SIGNALS:
    void alignmentChanged();
    void offsetChanged();
    void screenGeometryChanged();
    void thicknessChanged();
    void lengthChanged();
    void maximumLengthChanged();
    void minimumLengthChanged();
    void distanceChanged();
    void backgroundHintsChanged();
    void enabledBordersChanged();
    void floatingChanged();
    void minThicknessChanged();
    void geometryChanged();

    // QWindow does not have a property for screen. Adding this property requires re-implementing the signal
    void screenToFollowChanged(QScreen *screen);
    void visibilityModeChanged();
    void opacityModeChanged();
    void lengthModeChanged();
    void adaptiveOpacityEnabledChanged();
    void configRegionChanged();

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    void showConfigurationInterface(Plasma::Applet *applet) override;

private Q_SLOTS:
    void restore();
    void setAutoHideEnabled(bool autoHideEnabled);
    void showTemporarily();
    void refreshContainment();
    void refreshStatus(Plasma::Types::ItemStatus);
    void restoreAutoHide();
    void screenDestroyed(QObject *screen);
    void adaptToScreen();
    void handleQmlStatusChange(QQmlComponent::Status status);
    void updateMask();
    void updateEnabledBorders();
    void updatePadding();
    void updateFloating();
    void updateFloatingAnimationDuration();
    void updateShadows();
    void updateTouchingWindow();

private:
    int readConfigValueWithFallBack(const QString &key, int defaultValue);
    void positionPanel();
    void resizePanel();
    void integrateScreen();
    void updateEditModeLabel();
    bool containmentContainsPosition(const QPointF &point) const;
    QPointF positionAdjustedForContainment(const QPointF &point) const;
    bool edgeActivated() const;
    bool canSetStrut() const;
    QRect geometryByDistance(int distance, double floatingness) const;

    int m_offset;
    int m_maxLength;
    int m_minLength;
    int m_contentLength;
    int m_distance;
    int m_thickness;
    int m_bottomPadding;
    int m_topPadding;
    int m_leftPadding;
    int m_rightPadding;
    double m_floatingness = 0.0;
    QPropertyAnimation m_floatingnessAnimation;
    QSize m_targetSize;
    int m_bottomFloatingPadding;
    int m_topFloatingPadding;
    int m_leftFloatingPadding;
    int m_rightFloatingPadding;
    int m_minDrawingWidth;
    int m_minDrawingHeight;
    bool m_initCompleted;
    bool m_floating;
    bool m_containsMouse = false;
    bool m_fakeEventPending = false;
    bool m_touchingWindow = false;
    Qt::Alignment m_alignment;
    QPointer<PlasmaQuick::ConfigView> m_appletConfigView;
    QPointer<PlasmaQuick::PopupPlasmaWindow> m_panelConfigView = 0;
    ShellCorona *m_corona;
    QTimer m_strutsTimer;
    VisibilityMode m_visibilityMode;
    OpacityMode m_opacityMode;
    LengthMode m_lengthMode;
    Plasma::Theme m_theme;
    QTimer m_unhideTimer;
    Plasma::Types::BackgroundHints m_backgroundHints;
    KSvg::FrameSvg::EnabledBorders m_enabledBorders = KSvg::FrameSvg::AllBorders;
    LayerShellQt::Window *m_layerWindow = nullptr;
    QPointer<QScreen> m_lastScreen;
    QPointer<QScreen> m_screenToFollow;
    QMetaObject::Connection m_transientWindowVisibleWatcher;
    AutoHideScreenEdge *m_autoHideScreenEdge = nullptr;

    static const int STRUTSTIMERDELAY = 200;
};
