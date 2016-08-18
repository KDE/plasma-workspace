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

#ifndef SCRIPTENGINE
#define SCRIPTENGINE

#include <QScriptEngine>
#include <QScriptValue>

#include <QFontMetrics>

#include <kactivities/controller.h>

#include "../shellcorona.h"

namespace Plasma
{
    class Applet;
    class Containment;
} // namespace Plasma


namespace WorkspaceScripting
{

class Containment;

class ScriptEngine : public QScriptEngine
{
    Q_OBJECT

public:
    ScriptEngine(Plasma::Corona *corona, QObject *parent = 0);
    ~ScriptEngine() override;

    static QStringList pendingUpdateScripts(Plasma::Corona *corona);

    Plasma::Corona *corona() const;
    QScriptValue wrap(Plasma::Applet *w);
    virtual QScriptValue wrap(Plasma::Containment *c);
    QScriptValue wrap(Containment *c);
    virtual int defaultPanelScreen() const;

    static bool isPanel(const Plasma::Containment *c);
    static ScriptEngine *envFor(QScriptEngine *engine);

public Q_SLOTS:
    bool evaluateScript(const QString &script, const QString &path = QString());

Q_SIGNALS:
    void print(const QString &string);
    void printError(const QString &string);

private:
    void setupEngine();
    static QString onlyExec(const QString &commandLine);

    static QScriptValue createAPIForVersion(QScriptContext *context, QScriptEngine *engine);

    // Script API versions
    class V1;

    // helpers
    QStringList availableActivities() const;
    QList<Containment*> desktopsForActivity(const QString &id);
    Containment *createContainment(const QString &type, const QString &plugin);

    static int gridUnit();

private Q_SLOTS:
    void exception(const QScriptValue &value);

private:
    Plasma::Corona *m_corona;
    QScriptValue m_scriptSelf;
};

static const int PLASMA_DESKTOP_SCRIPTING_VERSION = 20;
}

#endif

