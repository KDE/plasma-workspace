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
    Q_PROPERTY(QRectF geometry WRITE setGeometry READ geometry)
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)
    Q_PROPERTY(QString globalShortcut WRITE setGlobalShortcut READ globalShorcut)
    Q_PROPERTY(bool locked READ locked WRITE setLocked)

public:
    Widget(Plasma::Applet *applet, QObject *parent = 0);
    ~Widget() override;

    uint id() const;
    QString type() const;

    int index() const;
    void setIndex(int index);

    QRectF geometry() const;
    void setGeometry(const QRectF &geometry);

    void setGlobalShortcut(const QString &shortcut);
    QString globalShorcut() const;

    Plasma::Applet *applet() const override;

public Q_SLOTS:
    void remove();
    void showConfigurationInterface();

    // from the applet interface
    QVariant readConfig(const QString &key, const QVariant &def = QString()) const override { return Applet::readConfig(key, def); }
    void writeConfig(const QString &key, const QVariant &value) override { Applet::writeConfig(key, value); }

    QVariant readGlobalConfig(const QString &key, const QVariant &def = QString()) const override { return Applet::readGlobalConfig(key, def); }
    void writeGlobalConfig(const QString &key, const QVariant &value) override { Applet::writeGlobalConfig(key, value); }

    void reloadConfig() override { Applet::reloadConfig(); }

private:
    class Private;
    Private * const d;
};

}

#endif

