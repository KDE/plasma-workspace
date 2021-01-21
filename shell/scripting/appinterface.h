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

#ifndef APPINTERFACE
#define APPINTERFACE

#include <QJSValue>
#include <QObject>
#include <QRectF>
#include <QStringList>

namespace Plasma
{
class Containment;
class Corona;
class Theme;
} // namespace Plasma

namespace WorkspaceScripting
{
class ScriptEngine;

class AppInterface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool locked READ coronaLocked WRITE lockCorona)
    Q_PROPERTY(bool hasBattery READ hasBattery)
    Q_PROPERTY(int screenCount READ screenCount)
    Q_PROPERTY(QList<int> activityIds READ activityIds)
    Q_PROPERTY(QList<int> panelIds READ panelIds)
    Q_PROPERTY(QStringList knownPanelTypes READ knownPanelTypes)
    Q_PROPERTY(QStringList knownActivityTypes READ knownActivityTypes)
    Q_PROPERTY(QStringList knownWidgetTypes READ knownWidgetTypes)
    Q_PROPERTY(QString theme READ theme WRITE setTheme)
    Q_PROPERTY(QString applicationVersion READ applicationVersion)
    Q_PROPERTY(QString platformVersion READ platformVersion)
    Q_PROPERTY(int scriptingVersion READ scriptingVersion)
    Q_PROPERTY(bool multihead READ multihead)
    Q_PROPERTY(bool multiheadScreen READ multihead)
    Q_PROPERTY(QString locale READ locale)
    Q_PROPERTY(QString language READ language)
    Q_PROPERTY(QString languageId READ languageId)

public:
    explicit AppInterface(ScriptEngine *env);

    bool hasBattery() const;
    int screenCount() const;
    QList<int> activityIds() const;
    QList<int> panelIds() const;

    QStringList knownWidgetTypes() const;
    QStringList knownActivityTypes() const;
    QStringList knownPanelTypes() const;
    QStringList knownContainmentTypes(const QString &type) const;

    QString applicationVersion() const;
    QString platformVersion() const;
    int scriptingVersion() const;

    QString theme() const;
    void setTheme(const QString &name);

    QString locale() const;
    QString language() const;
    QString languageId() const;

    bool multihead() const;
    int multiheadScreen() const;

    bool coronaLocked() const;

public Q_SLOTS:
    QJSValue screenGeometry(int screen) const;
    void lockCorona(bool locked);
    void sleep(int ms);

Q_SIGNALS:
    void print(const QString &string);

private:
    ScriptEngine *m_env;
    QStringList m_knownWidgets;
    Plasma::Theme *m_theme;
};

}

#endif
