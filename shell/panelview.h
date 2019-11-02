/*
 *  Copyright 2013 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PANELVIEW_H
#define PANELVIEW_H

#include <QPointer>
#include <Plasma/Theme>
#include <QTimer>

#include <PlasmaQuick/ContainmentView>
#include <PlasmaQuick/ConfigView>

class ShellCorona;

namespace KWayland
{
    namespace Client
    {
        class PlasmaShellSurface;
    }
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
     * height of horizontal panels, width of vertical panels
     */
    Q_PROPERTY(int thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)

    /**
     * width of horizontal panels, height of vertical panels
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
    Q_PROPERTY(Plasma::FrameSvg::EnabledBorders enabledBorders READ enabledBorders NOTIFY enabledBordersChanged)

    /**
     * information about the screen in which the panel is in
     */
    Q_PROPERTY(QScreen *screenToFollow READ screenToFollow WRITE setScreenToFollow NOTIFY screenToFollowChanged)

    /**
     *  how the panel behaves, visible, autohide etc.
     */
    Q_PROPERTY(VisibilityMode visibilityMode READ visibilityMode WRITE setVisibilityMode NOTIFY visibilityModeChanged)

public:

    enum VisibilityMode {
        NormalPanel = 0, /** default, always visible panel, the windowmanager reserves a places for it */
        AutoHide, /**the panel will be shownn only if the mouse cursor is on screen edges */
        LetWindowsCover, /** always visible, windows will go over the panel, no area reserved */
        WindowsGoBelow /** always visible, windows will go under the panel, no area reserved */
    };
    Q_ENUM(VisibilityMode)

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

    int length() const;
    void setLength(int value);

    int maximumLength() const;
    void setMaximumLength(int length);

    int minimumLength() const;
    void setMinimumLength(int length);

    int distance() const;
    void setDistance(int dist);

    Plasma::Types::BackgroundHints backgroundHints() const;
    void setBackgroundHints(Plasma::Types::BackgroundHints hint);

    Plasma::FrameSvg::EnabledBorders enabledBorders() const;

    VisibilityMode visibilityMode() const;
    void setVisibilityMode(PanelView::VisibilityMode mode);

    /**
     * @returns the geometry of the panel given a distance
     */
    QRect geometryByDistance(int distance) const;

    /* Both Shared with script/panel.cpp */
    static KConfigGroup panelConfig(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen);
    static KConfigGroup panelConfigDefaults(ShellCorona *corona, Plasma::Containment *containment, QScreen *screen);

    void updateStruts();

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen* screen);
    QScreen* screenToFollow() const;

protected:
    void resizeEvent(QResizeEvent *ev) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *ev) override;
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

    //QWindow does not have a property for screen. Adding this property requires re-implementing the signal
    void screenToFollowChanged(QScreen *screen);
    void visibilityModeChanged();

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    void showConfigurationInterface(Plasma::Applet *applet) override;

private Q_SLOTS:
    void positionPanel();
    void restore();
    void setAutoHideEnabled(bool autoHideEnabled);
    void showTemporarily();
    void containmentChanged();
    void statusChanged(Plasma::Types::ItemStatus);
    void restoreAutoHide();
    void screenDestroyed(QObject* screen);
    void adaptToScreen();
    void handleQmlStatusChange(QQmlComponent::Status status);
    void updateMask();
    void updateEnabledBorders();    

private:
    int readConfigValueWithFallBack(const QString &key, int defaultValue);
    void resizePanel();
    void integrateScreen();
    bool containmentContainsPosition(const QPointF &point) const;
    QPointF positionAdjustedForContainment(const QPointF &point) const;
    void setupWaylandIntegration();
    void visibilityModeToWayland();
    bool edgeActivated() const;
    bool canSetStrut() const;

    int m_offset;
    int m_maxLength;
    int m_minLength;
    int m_contentLength;
    int m_distance;
    int m_thickness;
    bool m_initCompleted;
    bool m_containsMouse = false;
    Qt::Alignment m_alignment;
    QPointer<PlasmaQuick::ConfigView> m_panelConfigView;
    ShellCorona *m_corona;
    QTimer m_strutsTimer;
    VisibilityMode m_visibilityMode;
    Plasma::Theme m_theme;
    QTimer m_positionPaneltimer;
    QTimer m_unhideTimer;
    Plasma::Types::BackgroundHints m_backgroundHints;
    Plasma::FrameSvg::EnabledBorders m_enabledBorders = Plasma::FrameSvg::AllBorders;
    KWayland::Client::PlasmaShellSurface *m_shellSurface;
    QPointer<QScreen> m_lastScreen;
    QPointer<QScreen> m_screenToFollow;
    QMetaObject::Connection m_transientWindowVisibleWatcher;

    static const int STRUTSTIMERDELAY = 200;
};

#endif // PANELVIEW_H
