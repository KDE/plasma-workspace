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

#include <QJSValue>
#include <QWeakPointer>

#include "applet.h"

namespace Plasma
{
class Containment;
} // namespace Plasma

class ShellCorona;

namespace WorkspaceScripting
{
class ScriptEngine;

class Containment : public Applet
{
    Q_OBJECT
    /// FIXME: add NOTIFY
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QStringList configKeys READ configKeys)
    Q_PROPERTY(QStringList configGroups READ configGroups)
    Q_PROPERTY(QStringList globalConfigKeys READ globalConfigKeys)
    Q_PROPERTY(QStringList globalConfigGroups READ globalConfigGroups)
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)
    Q_PROPERTY(QString wallpaperPlugin READ wallpaperPlugin WRITE setWallpaperPlugin)
    Q_PROPERTY(QString wallpaperMode READ wallpaperMode WRITE setWallpaperMode)
    Q_PROPERTY(bool locked READ locked WRITE setLocked)
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString formFactor READ formFactor)
    Q_PROPERTY(QList<int> widgetIds READ widgetIds)
    Q_PROPERTY(int screen READ screen)
    Q_PROPERTY(int id READ id)

public:
    explicit Containment(Plasma::Containment *containment, ScriptEngine *parent);
    ~Containment() override;

    uint id() const;
    QString type() const;
    QString formFactor() const;
    QList<int> widgetIds() const;

    int screen() const;

    Plasma::Applet *applet() const override;
    Plasma::Containment *containment() const;

    QString wallpaperPlugin() const;
    void setWallpaperPlugin(const QString &wallpaperPlugin);
    QString wallpaperMode() const;
    void setWallpaperMode(const QString &wallpaperMode);

    Q_INVOKABLE QJSValue widgetById(const QJSValue &paramId = QJSValue()) const;
    Q_INVOKABLE QJSValue
    addWidget(const QJSValue &v = QJSValue(), qreal x = -1, qreal y = -1, qreal w = -1, qreal h = -1, const QVariantList &args = QVariantList());
    Q_INVOKABLE QJSValue widgets(const QString &widgetType = QString()) const;

public Q_SLOTS:
    void remove();
    void showConfigurationInterface();

protected:
    ShellCorona *corona() const;

private:
    class Private;
    Private *const d;
};

}

#endif
