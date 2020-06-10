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

#ifndef WIDGET
#define WIDGET

#include <QWeakPointer>
#include <QJSValue>

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
    //We pass our js based QRect wrapper instead of a simple QRectF
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
    void setUserBackgroundHints(QString hint);

    Plasma::Applet *applet() const override;

public Q_SLOTS:
    void remove();
    void showConfigurationInterface();

private:
    class Private;
    Private * const d;
};

}

#endif

