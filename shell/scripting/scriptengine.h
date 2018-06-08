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

#include <QJSEngine>
#include <QJSValue>

#include <QFontMetrics>

#include <kactivities/controller.h>

#include "../shellcorona.h"

namespace Plasma
{
    class Applet;
    class Containment;
} // namespace Plasma

class KLocalizedContext;

namespace WorkspaceScripting
{
class AppInterface;
class Containment;
class V1;


class ScriptEngine : public QJSEngine
{
    Q_OBJECT

public:
    explicit ScriptEngine(Plasma::Corona *corona, QObject *parent = nullptr);
    ~ScriptEngine() override;

    QString errorString() const;

    static QStringList pendingUpdateScripts(Plasma::Corona *corona);

    Plasma::Corona *corona() const;
    QJSValue wrap(Plasma::Applet *w);
    QJSValue wrap(Plasma::Containment *c);
    virtual int defaultPanelScreen() const;
    QJSValue newError(const QString &message);

    static bool isPanel(const Plasma::Containment *c);

    Plasma::Containment *createContainment(const QString &type, const QString &plugin);

public Q_SLOTS:
    bool evaluateScript(const QString &script, const QString &path = QString());

Q_SIGNALS:
    void print(const QString &string);
    void printError(const QString &string);

private:
    void setupEngine();
    static QString onlyExec(const QString &commandLine);

    // Script API versions
    class V1;

    // helpers
    QStringList availableActivities() const;
    QList<Containment*> desktopsForActivity(const QString &id);
    Containment *createContainmentWrapper(const QString &type, const QString &plugin);

private Q_SLOTS:
    void exception(const QJSValue &value);

private:
    Plasma::Corona *m_corona;
    ScriptEngine::V1 *m_globalScriptEngineObject;
    KLocalizedContext *m_localizedContext;
    AppInterface *m_appInterface;
    QJSValue m_scriptSelf;
    QString m_errorString;
};

static const int PLASMA_DESKTOP_SCRIPTING_VERSION = 20;
}

#endif

