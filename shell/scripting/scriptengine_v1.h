/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2018 Marco Martin <mart@kde.org>
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

#ifndef SCRIPTENGINE_V1
#define SCRIPTENGINE_V1

#include <QJSValue>
#include <QObject>

#include <QFontMetrics>

#include <kactivities/controller.h>

#include "../shellcorona.h"
#include "scriptengine.h"

namespace WorkspaceScripting
{
class ScriptEngine::V1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int gridUnit READ gridUnit CONSTANT)

public:
    V1(ScriptEngine *parent);
    ~V1();
    int gridUnit() const;

    Q_INVOKABLE QJSValue getApiVersion(const QJSValue &param);
    Q_INVOKABLE QJSValue desktopById(const QJSValue &id = QJSValue()) const;
    Q_INVOKABLE QJSValue desktopsForActivity(const QJSValue &id = QJSValue()) const;
    Q_INVOKABLE QJSValue desktopForScreen(const QJSValue &screen = QJSValue()) const;
    Q_INVOKABLE QJSValue screenForConnector(const QJSValue &param = QJSValue()) const;
    Q_INVOKABLE QJSValue createActivity(const QJSValue &nameParam = QJSValue(), const QString &plugin = QString());
    Q_INVOKABLE QJSValue setCurrentActivity(const QJSValue &id = QJSValue());
    Q_INVOKABLE QJSValue setActivityName(const QJSValue &idParam = QJSValue(), const QJSValue &nameParam = QJSValue());
    Q_INVOKABLE QJSValue activityName(const QJSValue &idParam = QJSValue()) const;
    Q_INVOKABLE QString currentActivity() const;
    Q_INVOKABLE QJSValue activities() const;
    Q_INVOKABLE QJSValue loadSerializedLayout(const QJSValue &data = QJSValue());
    Q_INVOKABLE QJSValue panelById(const QJSValue &idParam = QJSValue()) const;
    Q_INVOKABLE QJSValue desktops() const;
    Q_INVOKABLE QJSValue panels() const;
    Q_INVOKABLE bool fileExists(const QString &path = QString()) const;
    Q_INVOKABLE bool loadTemplate(const QString &layout = QString());
    Q_INVOKABLE bool applicationExists(const QString &application = QString()) const;
    Q_INVOKABLE QJSValue defaultApplication(const QString &application = QString(), bool storageId = false) const;
    Q_INVOKABLE QJSValue applicationPath(const QString &application = QString()) const;
    Q_INVOKABLE QJSValue userDataPath(const QString &type = QString(), const QString &path = QString()) const;
    Q_INVOKABLE QJSValue knownWallpaperPlugins(const QString &formFactor = QString()) const;

    Q_INVOKABLE void setImmutability(const QString &immutability = QString());
    Q_INVOKABLE QString immutability() const;
    Q_INVOKABLE QJSValue createContainment(const QString &type, const QString &defautPlugin, const QString &plugin = QString());

    // for ctors
    Q_INVOKABLE QJSValue newPanel(const QString &plugin = QStringLiteral("org.kde.panel"));
    Q_INVOKABLE QJSValue configFile(const QJSValue &config = QJSValue(), const QString &group = QString());

Q_SIGNALS:
    void print(const QJSValue &param);

private:
    ScriptEngine *m_engine;
};

}

#endif
