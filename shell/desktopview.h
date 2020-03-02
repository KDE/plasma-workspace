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

#ifndef DESKTOPVIEW_H
#define DESKTOPVIEW_H


#include <PlasmaQuick/ContainmentView>
#include <PlasmaQuick/ConfigView>
#include <QPointer>

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

    //What kind of plasma session we're in: are we in a full workspace, an application?...
    Q_PROPERTY(SessionType sessionType READ sessionType CONSTANT)

    Q_PROPERTY(QVariantMap candidateContainments READ candidateContainmentsGraphicItems NOTIFY candidateContainmentsChanged)

public:
    enum WindowType {
        Window, /** The window is a normal resizable window with titlebar and appears in the taskbar */
        FullScreen, /** The window is fullscreen and goes over all the other windows */
        Desktop, /** The window is the desktop layer, under everything else, doesn't appear in the taskbar */
        WindowedDesktop /** full screen and borderless as Desktop, but can be brought in front and appears in the taskbar */
    };
    Q_ENUM(WindowType)

    enum SessionType {
        ApplicationSession, /** our session is a normal application */
        ShellSession /** We are running as the primary user interface of this machine */
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
    void showEvent(QShowEvent*) override;

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
    void screenGeometryChanged();

Q_SIGNALS:
    void stayBehindChanged();
    void windowTypeChanged();
    void candidateContainmentsChanged();
    void geometryChanged();

private:
    void coronaPackageChanged(const KPackage::Package &package);
    void ensureWindowType();
    void setupWaylandIntegration();
    bool handleKRunnerTextInput(QKeyEvent *e);

    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QScreen> m_oldScreen;
    QPointer<QScreen> m_screenToFollow;
    WindowType m_windowType;
    KWayland::Client::PlasmaShellSurface *m_shellSurface;
    QString m_krunnerText;
};

#endif // DESKTOPVIEW_H
