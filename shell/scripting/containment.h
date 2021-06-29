/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
