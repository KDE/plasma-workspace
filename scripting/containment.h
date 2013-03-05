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

#ifndef CONTAINMENT
#define CONTAINMENT

#include <QScriptContext>
#include <QScriptValue>
#include <QWeakPointer>

#include "applet.h"

#include "../plasmagenericshell_export.h"

namespace Plasma
{
    class Containment;
} // namespace Plasma

namespace WorkspaceScripting
{

class PLASMAGENERICSHELL_EXPORT Containment : public Applet
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QStringList configKeys READ configKeys)
    Q_PROPERTY(QStringList configGroups READ configGroups)
    Q_PROPERTY(QStringList globalConfigKeys READ globalConfigKeys)
    Q_PROPERTY(QStringList globalConfigGroups READ globalConfigGroups)
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString wallpaperPlugin READ wallpaperPlugin WRITE setWallpaperPlugin)
    Q_PROPERTY(QString wallpaperMode READ wallpaperMode WRITE setWallpaperMode)
    Q_PROPERTY(bool locked READ locked WRITE setLocked)
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString formFactor READ formFactor)
    Q_PROPERTY(QList<int> widgetIds READ widgetIds)
    Q_PROPERTY(int screen READ screen WRITE setScreen)
    Q_PROPERTY(int desktop READ desktop WRITE setDesktop)
    Q_PROPERTY(int id READ id)

public:
    Containment(Plasma::Containment *containment, QObject *parent = 0);
    ~Containment();

    uint id() const;
    QString type() const;
    QString formFactor() const;
    QList<int> widgetIds() const;

    QString name() const;
    void setName(const QString &name);

    int desktop() const;
    void setDesktop(int desktop);

    int screen() const;
    void setScreen(int screen);

    Plasma::Applet *applet() const;
    Plasma::Containment *containment() const;

    QString wallpaperPlugin() const;
    void setWallpaperPlugin(const QString &wallpaperPlugin);
    QString wallpaperMode() const;
    void setWallpaperMode(const QString &wallpaperMode);

    static QScriptValue widgetById(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue addWidget(QScriptContext *context, QScriptEngine *engine);
    static QScriptValue widgets(QScriptContext *context, QScriptEngine *engine);

public Q_SLOTS:
    void remove();
    void showConfigurationInterface();

    // from the applet interface
    QVariant readConfig(const QString &key, const QVariant &def = QString()) const { return Applet::readConfig(key, def); }
    void writeConfig(const QString &key, const QVariant &value) { Applet::writeConfig(key, value); }

    QVariant readGlobalConfig(const QString &key, const QVariant &def = QString()) const { return Applet::readGlobalConfig(key, def); }
    void writeGlobalConfig(const QString &key, const QVariant &value) { Applet::writeGlobalConfig(key, value); }

    void reloadConfig() { Applet::reloadConfig(); }

private:
    class Private;
    Private * const d;
};

}

#endif

