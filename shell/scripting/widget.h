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
class Applet;
} // namespace Plasma

namespace WorkspaceScripting
{
class Widget : public Applet
{
    Q_OBJECT
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QStringList configKeys READ configKeys)
    Q_PROPERTY(QStringList configGroups READ configGroups)
    Q_PROPERTY(QStringList globalConfigKeys READ globalConfigKeys)
    Q_PROPERTY(QStringList globalConfigGroups READ globalConfigGroups)
    Q_PROPERTY(int index WRITE setIndex READ index)
    // We pass our js based QRect wrapper instead of a simple QRectF
    Q_PROPERTY(QJSValue geometry WRITE setGeometry READ geometry)
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)
    Q_PROPERTY(QString globalShortcut WRITE setGlobalShortcut READ globalShorcut)
    Q_PROPERTY(bool locked READ locked WRITE setLocked)
    Q_PROPERTY(QString userBackgroundHints WRITE setUserBackgroundHints READ userBackgroundHints)

public:
    explicit Widget(Plasma::Applet *applet, ScriptEngine *parent = nullptr);
    ~Widget() override;

    uint id() const;
    QString type() const;

    int index() const;
    void setIndex(int index);

    QJSValue geometry() const;
    void setGeometry(const QJSValue &geometry);

    void setGlobalShortcut(const QString &shortcut);
    QString globalShorcut() const;

    QString userBackgroundHints() const;
    void setUserBackgroundHints(const QString &hint);

    Plasma::Applet *applet() const override;

public Q_SLOTS:
    void remove();
    void showConfigurationInterface();

private:
    class Private;
    Private *const d;
};

}
