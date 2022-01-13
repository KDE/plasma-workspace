/*
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QQuickItem>
#include <QUrl>

namespace Plasma
{
class Applet;
class Containment;
}

class ContainmentInterface : public QObject
{
    Q_OBJECT

public:
    enum Target {
        Desktop = 0,
        Panel,
        TaskManager,
    };

    Q_ENUM(Target)

    explicit ContainmentInterface(QObject *parent = nullptr);
    ~ContainmentInterface() override;

    static Q_INVOKABLE bool mayAddLauncher(QObject *appletInterface, Target target, const QString &entryPath = QString());

    static Q_INVOKABLE void addLauncher(QObject *appletInterface, Target target, const QString &entryPath);

    static Q_INVOKABLE QObject *screenContainment(QObject *appletInterface);
    static Q_INVOKABLE bool screenContainmentMutable(QObject *appletInterface);
    static Q_INVOKABLE void ensureMutable(Plasma::Containment *containment);

private:
    template<class UnaryPredicate>
    static QQuickItem *findPlasmaGraphicObjectChildIf(const Plasma::Applet *applet, UnaryPredicate predicate);
    static QQuickItem *firstPlasmaGraphicObjectChild(const Plasma::Applet *applet);

    static Plasma::Applet *findTaskManagerApplet(Plasma::Containment *containment);
    static QStringList m_knownTaskManagers;
};
