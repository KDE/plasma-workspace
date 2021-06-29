/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    QList<Containment *> desktopsForActivity(const QString &id);
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
