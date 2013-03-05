/*
 *   Copyright 2010 Aaron Seigo <aseigo@kde.org>
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

#ifndef APPLET
#define APPLET

#include <QObject>
#include <QWeakPointer>

#include <KConfigGroup>

#include "../plasmagenericshell_export.h"

namespace Plasma
{
    class Applet;
} // namespace Plasma

namespace WorkspaceScripting
{

class PLASMAGENERICSHELL_EXPORT Applet : public QObject
{
    Q_OBJECT

public:
    Applet(QObject *parent = 0);
    ~Applet();

    QStringList configKeys() const;
    QStringList configGroups() const;

    void setCurrentConfigGroup(const QStringList &groupNames);
    QStringList currentConfigGroup() const;

    QStringList globalConfigKeys() const;
    QStringList globalConfigGroups() const;

    void setCurrentGlobalConfigGroup(const QStringList &groupNames);
    QStringList currentGlobalConfigGroup() const;

    QString version() const;

    void setLocked(bool locked);
    bool locked() const;

    bool wallpaperConfigDirty() const;

    virtual Plasma::Applet *applet() const;

protected:
    void reloadConfigIfNeeded();

public Q_SLOTS:
    virtual QVariant readConfig(const QString &key, const QVariant &def = QString()) const;
    virtual void writeConfig(const QString &key, const QVariant &value);
    virtual QVariant readGlobalConfig(const QString &key, const QVariant &def = QString()) const;
    virtual void writeGlobalConfig(const QString &key, const QVariant &value);
    virtual void reloadConfig();

private:
    class Private;
    Private * const d;
};

}

#endif

