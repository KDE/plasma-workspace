/*
    SPDX-FileCopyrightText: 2010 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QJSValue>
#include <QObject>
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
    Private *const d;
};

}
