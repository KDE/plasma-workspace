/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "appinterface.h"
#include <config-X11.h>

#include <QEventLoop>
#include <QTimer>

#include <Solid/Battery>
#include <Solid/Device>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>
#include <Plasma/PluginLoader>
#include <Plasma/Theme>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <fixx11h.h>
#endif

#include "scriptengine.h"

namespace WorkspaceScripting
{

AppInterface::AppInterface(ScriptEngine *env)
    : QObject(env),
      m_env(env)
{
    m_theme = new Plasma::Theme(this);
}

int AppInterface::screenCount() const
{
    return m_env->corona()->numScreens();
}

QRectF AppInterface::screenGeometry(int screen) const
{
    return m_env->corona()->screenGeometry(screen);
}

QList<int> AppInterface::activityIds() const
{
    //FIXME: the ints could overflow since Applet::id() returns a uint,
    //       however QScript deals with QList<uint> very, very poorly
    QList<int> containments;

    foreach (Plasma::Containment *c, m_env->corona()->containments()) {
        if (!ScriptEngine::isPanel(c)) {
            containments.append(c->id());
        }
    }

    return containments;
}

QList<int> AppInterface::panelIds() const
{
    //FIXME: the ints could overflow since Applet::id() returns a uint,
    //       however QScript deals with QList<uint> very, very poorly
    QList<int> panels;

    foreach (Plasma::Containment *c, m_env->corona()->containments()) {
        //qDebug() << "checking" << (QObject*)c << isPanel(c);
        if (ScriptEngine::isPanel(c)) {
            panels.append(c->id());
        }
    }

    return panels;
}

QString AppInterface::applicationVersion() const
{
    return qApp->applicationVersion();
}

QString AppInterface::platformVersion() const
{
    return 0;//KDE::versionString();
}

int AppInterface::scriptingVersion() const
{
    return PLASMA_DESKTOP_SCRIPTING_VERSION;
}

QString AppInterface::theme() const
{
    return m_theme->themeName();
}

void AppInterface::setTheme(const QString &name)
{
    m_theme->setThemeName(name);
}

QString AppInterface::locale() const
{
    return QLocale::system().name();
}

QString AppInterface::language() const
{
    return QLocale::system().languageToString(QLocale::system().language());
}

QString AppInterface::languageId() const
{
    return QLocale::system().bcp47Name().section('-', 0, 0);
}

bool AppInterface::multihead() const
{
#ifdef Q_OS_WIN
    return GetSystemMetrics(SM_CMONITORS) > 1;
#else
    QByteArray multiHead = qgetenv("KDE_MULTIHEAD");
    if (!multiHead.isEmpty()) {
        return (multiHead.toLower() == "true");
    }
    return false;
#endif
}

int AppInterface::multiheadScreen() const
{
    int id = -1;

#ifdef HAVE_X11
    if (multihead()) {
        // with multihead, we "lie" and say that screen 0 is the default screen, in fact, we pretend
        // we have only one screen at all
        Display *dpy = XOpenDisplay(NULL);
        if (dpy) {
            id = DefaultScreen(dpy);
            XCloseDisplay(dpy);
        }
    }
#endif

    return id;
}

void AppInterface::lockCorona(bool locked)
{
    m_env->corona()->setImmutability(locked ? Plasma::Types::UserImmutable : Plasma::Types::Mutable);
}

bool AppInterface::coronaLocked() const
{
    return m_env->corona()->immutability() != Plasma::Types::Mutable;
}

void AppInterface::sleep(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

bool AppInterface::hasBattery() const
{
    QList<Solid::Device> batteryDevices = Solid::Device::listFromType(Solid::DeviceInterface::Battery);

    for (auto device: batteryDevices) {
        Solid::Battery *battery = device.as<Solid::Battery>();
        // check for _both_ primary and power supply status
        // apparently some devices misreport as "primary", and we don't
        // want to trigger on just having UPC connected to a desktop box
        if (battery &&
            battery->type() == Solid::Battery::PrimaryBattery &&
            battery->isPowerSupply()) {
            return true;
        }
    }

    return false;
}

QStringList AppInterface::knownWidgetTypes() const
{
    if (m_knownWidgets.isEmpty()) {
        QStringList widgets;
        KPluginInfo::List infoLs = Plasma::PluginLoader::self()->listAppletInfo(QString());

        foreach (const KPluginInfo &info, infoLs) {
            widgets.append(info.pluginName());
        }

        const_cast<AppInterface *>(this)->m_knownWidgets = widgets;
    }

    return m_knownWidgets;
}

QStringList AppInterface::knownActivityTypes() const
{
    return knownContainmentTypes(QStringLiteral("desktop"));
}

QStringList AppInterface::knownPanelTypes() const
{
    return knownContainmentTypes(QStringLiteral("panel"));
}

QStringList AppInterface::knownContainmentTypes(const QString &type) const
{
    QStringList containments;
    KPluginInfo::List infoLs = Plasma::PluginLoader::listContainmentsOfType(type);

    foreach (const KPluginInfo &info, infoLs) {
        containments.append(info.pluginName());
    }

    return containments;
}

}



