/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <PlasmaQuick/ConfigView>
#include <PlasmaQuick/ContainmentView>
#include <QPointer>

#include <KConfigWatcher>

namespace KWayland
{
namespace Client
{
class PlasmaShellSurface;
}
}

class DesktopView : public PlasmaQuick::ContainmentView
{
    Q_OBJECT

    Q_PROPERTY(WindowType windowType READ windowType WRITE setWindowType NOTIFY windowTypeChanged)

    // What kind of plasma session we're in: are we in a full workspace, an application?...
    Q_PROPERTY(SessionType sessionType READ sessionType CONSTANT)

    Q_PROPERTY(QVariantMap candidateContainments READ candidateContainmentsGraphicItems NOTIFY candidateContainmentsChanged)

    /**
     * Whether the desktop is used in accent color extraction
     *
     * @note When usedInAccentColor becomes @c true, \Kirigami.ImageColors
     * will be loaded and update the accent color, and \setAccentColor will
     * be called
     */
    Q_PROPERTY(bool usedInAccentColor READ usedInAccentColor NOTIFY usedInAccentColorChanged)

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

public:
    enum WindowType {
        Window, /** The window is a normal resizable window with titlebar and appears in the taskbar */
        FullScreen, /** The window is fullscreen and goes over all the other windows */
        Desktop, /** The window is the desktop layer, under everything else, doesn't appear in the taskbar */
        WindowedDesktop, /** full screen and borderless as Desktop, but can be brought in front and appears in the taskbar */
    };
    Q_ENUM(WindowType)

    enum SessionType {
        ApplicationSession, /** our session is a normal application */
        ShellSession, /** We are running as the primary user interface of this machine */
    };
    Q_ENUM(SessionType)

    explicit DesktopView(Plasma::Corona *corona, QScreen *targetScreen = nullptr);
    ~DesktopView() override;

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen *screen);
    QScreen *screenToFollow() const;

    void adaptToScreen();
    void showEvent(QShowEvent *) override;

    bool usedInAccentColor() const;

    QColor accentColor() const;
    void setAccentColor(const QColor &);

    WindowType windowType() const;
    void setWindowType(WindowType type);

    SessionType sessionType() const;

    QVariantMap candidateContainmentsGraphicItems() const;

    Q_INVOKABLE QString fileFromPackage(const QString &key, const QString &fileName);

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    void showConfigurationInterface(Plasma::Applet *applet) override;

private Q_SLOTS:
    void slotContainmentChanged();
    void slotScreenChanged(int newId);
    void screenGeometryChanged();

Q_SIGNALS:
    void stayBehindChanged();
    void windowTypeChanged();
    void candidateContainmentsChanged();
    void geometryChanged();
    void usedInAccentColorChanged();
    void accentColorChanged(const QColor &accentColor);

private:
    void coronaPackageChanged(const KPackage::Package &package);
    void ensureWindowType();
    void setupWaylandIntegration();
    void setAccentColorFromWallpaper(const QColor &accentColor);
    bool handleKRunnerTextInput(QKeyEvent *e);

    QColor m_accentColor;
    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QScreen> m_screenToFollow;
    WindowType m_windowType;
    KWayland::Client::PlasmaShellSurface *m_shellSurface;
    QString m_krunnerText;

    // KRunner config
    KConfigWatcher::Ptr m_configWatcher;
    bool m_activateKRunnerWhenTypingOnDesktop;

    // Accent color config
    Plasma::Containment *m_containment = nullptr;
    int m_containmentScreenId = -1;
};
