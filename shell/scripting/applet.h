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
#include <QJSValue>
#include <QWeakPointer>

#include <kconfiggroup.h>


namespace Plasma
{
    class Applet;
} // namespace Plasma

namespace WorkspaceScripting
{

class ScriptEngine;

class Applet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)

public:
    explicit Applet(ScriptEngine *parent);
    ~Applet() override;

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

    virtual Plasma::Applet *applet() const;

    ScriptEngine *engine() const;

protected:
    void reloadConfigIfNeeded();

public Q_SLOTS:
    virtual QVariant readConfig(const QString &key, const QJSValue &def = QString()) const;
    virtual void writeConfig(const QString &key, const QJSValue &value);
    virtual QVariant readGlobalConfig(const QString &key, const QJSValue &def = QString()) const;
    virtual void writeGlobalConfig(const QString &key, const QJSValue &value);
    virtual void reloadConfig();

private:
    class Private;
    Private * const d;
};

}

#endif

