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

#ifndef DESKTOVIEW_H
#define DESKTOVIEW_H


#include "plasmaquick/view.h"
#include "panelconfigview.h"
#include <QtCore/qpointer.h>

class ShellCorona;

class DesktopView : public PlasmaQuick::View
{
    Q_OBJECT
    //the qml part doesn't need to be able to write it, hide for now
    Q_PROPERTY(bool dashboardShown READ isDashboardShown NOTIFY dashboardShownChanged)

    Q_PROPERTY(WindowType windowType READ windowType WRITE setWindowType NOTIFY windowTypeChanged)

public:
    enum WindowType {
        Window, /** The window is a normal resizable window with titlebar and appears in the taskbar */
        FullScreen, /** The window is fullscreen and goes over all the other windows */
        Desktop, /** The window is the desktop layer, under everything else, doesn't appear in the taskbar */
        WindowedDesktop /** full screen and borderless as Desktop, but can be brought in front and appears in the taskbar */
    };
    Q_ENUMS(WindowType)

    explicit DesktopView(Plasma::Corona *corona);
    virtual ~DesktopView();

    void setDashboardShown(bool shown);
    bool isDashboardShown() const;

    void adaptToScreen();
    virtual void showEvent(QShowEvent*);

    WindowType windowType() const;
    void setWindowType(WindowType type);

protected:
    bool event(QEvent *e);
    void keyPressEvent(QKeyEvent *e);

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    virtual void showConfigurationInterface(Plasma::Applet *applet);
    void screenDestroyed(QObject* screen);

Q_SIGNALS:
    void stayBehindChanged();
    void dashboardShownChanged();
    void windowTypeChanged();

private:
    void coronaPackageChanged(const Plasma::Package &package);
    void ensureWindowType();

    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QScreen> m_oldScreen;
    bool m_dashboardShown : 1;
    WindowType m_windowType;
};

#endif // DESKTOVIEW_H
