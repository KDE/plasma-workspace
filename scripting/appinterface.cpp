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

#include <QEventLoop>
#include <QTimer>

#include <KGlobalSettings>

#include <Plasma/Containment>
#include <Plasma/Corona>
#include <Plasma/DataEngineManager>
#include <Plasma/Theme>

#ifdef Q_WS_X11
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
    //       however QScript deals with QList<uint> very, very poory
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
    //       however QScript deals with QList<uint> very, very poory
    QList<int> panels;

    foreach (Plasma::Containment *c, m_env->corona()->containments()) {
        //kDebug() << "checking" << (QObject*)c << isPanel(c);
        if (ScriptEngine::isPanel(c)) {
            panels.append(c->id());
        }
    }

    return panels;
}

QString AppInterface::applicationVersion() const
{
    return KGlobal::mainComponent().aboutData()->version();
}

QString AppInterface::platformVersion() const
{
    return KDE::versionString();
}

int AppInterface::scriptingVersion() const
{
    return PLASMA_DESKTOP_SCRIPTING_VERSION;
}

QString AppInterface::theme() const
{
    return Plasma::Theme::defaultTheme()->themeName();
}

void AppInterface::setTheme(const QString &name)
{
    Plasma::Theme::defaultTheme()->setThemeName(name);
}

bool AppInterface::multihead() const
{
    return KGlobalSettings::isMultiHead();
}

int AppInterface::multiheadScreen() const
{
    int id = -1;

#ifdef Q_WS_X11
    if (KGlobalSettings::isMultiHead()) {
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
    m_env->corona()->setImmutability(locked ? Plasma::UserImmutable : Plasma::Mutable);
}

bool AppInterface::coronaLocked() const
{
    return m_env->corona()->immutability() != Plasma::Mutable;
}

void AppInterface::sleep(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, SLOT(quit()));
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

bool AppInterface::hasBattery() const
{
  Plasma::DataEngineManager *engines = Plasma::DataEngineManager::self();
  Plasma::DataEngine *power = engines->loadEngine("powermanagement");

  const QStringList batteries = power->query("Battery")["Sources"].toStringList();
  engines->unloadEngine("powermanagement");
  return !batteries.isEmpty();
}

QStringList AppInterface::knownWidgetTypes() const
{
    if (m_knownWidgets.isEmpty()) {
        QStringList widgets;
        KPluginInfo::List infoLs = Plasma::Applet::listAppletInfo();

        foreach (const KPluginInfo &info, infoLs) {
            widgets.append(info.pluginName());
        }

        const_cast<AppInterface *>(this)->m_knownWidgets = widgets;
    }

    return m_knownWidgets;
}

QStringList AppInterface::knownActivityTypes() const
{
    return knownContainmentTypes("desktop");
}

QStringList AppInterface::knownPanelTypes() const
{
    return knownContainmentTypes("panel");
}

QStringList AppInterface::knownContainmentTypes(const QString &type) const
{
    QStringList containments;
    KPluginInfo::List infoLs = Plasma::Containment::listContainmentsOfType(type);

    foreach (const KPluginInfo &info, infoLs) {
        containments.append(info.pluginName());
    }

    return containments;
}

}

#include "appinterface.moc"

