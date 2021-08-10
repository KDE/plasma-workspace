/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plasmaautostart.h"

#include <KConfigGroup>
#include <KDesktopFile>

#include <QCoreApplication>
#include <QDir>
#include <QFile>

class PlasmaAutostartPrivate
{
public:
    PlasmaAutostartPrivate()
        : df(nullptr)
        , copyIfNeededChecked(false)
    {
    }

    ~PlasmaAutostartPrivate()
    {
        delete df;
    }

    void copyIfNeeded();

    QString name;
    KDesktopFile *df;
    bool copyIfNeededChecked;
};

void PlasmaAutostartPrivate::copyIfNeeded()
{
    if (copyIfNeededChecked) {
        return;
    }

    const QString local = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/autostart/") + name;

    if (!QFile::exists(local)) {
        const QString global = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("autostart/") + name);
        if (!global.isEmpty()) {
            KDesktopFile *newDf = df->copyTo(local);
            delete df;
            delete newDf; // Force sync-to-disk
            df = new KDesktopFile(QStandardPaths::GenericConfigLocation, QStringLiteral("autostart/") + name); // Recreate from disk
        }
    }

    copyIfNeededChecked = true;
}

PlasmaAutostart::PlasmaAutostart(const QString &entryName, QObject *parent)
    : QObject(parent)
    , d(new PlasmaAutostartPrivate)
{
    const bool isAbsolute = QDir::isAbsolutePath(entryName);
    if (isAbsolute) {
        d->name = entryName.mid(entryName.lastIndexOf(QLatin1Char('/')) + 1);
    } else {
        if (entryName.isEmpty()) {
            d->name = QCoreApplication::applicationName();
        } else {
            d->name = entryName;
        }

        if (!d->name.endsWith(QLatin1String(".desktop"))) {
            d->name.append(QLatin1String(".desktop"));
        }
    }

    const QString path = isAbsolute ? entryName : QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("autostart/") + d->name);
    if (path.isEmpty()) {
        // just a new KDesktopFile, since we have nothing to use
        d->df = new KDesktopFile(QStandardPaths::GenericConfigLocation, QLatin1String("autostart/") + d->name);
        d->copyIfNeededChecked = true;
    } else {
        d->df = new KDesktopFile(path);
    }
}

PlasmaAutostart::~PlasmaAutostart() = default;

void PlasmaAutostart::setAutostarts(bool autostart)
{
    bool currentAutostartState = !d->df->desktopGroup().readEntry("Hidden", false);
    if (currentAutostartState == autostart) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeEntry("Hidden", !autostart);
}

bool PlasmaAutostart::autostarts(const QString &environment, Conditions check) const
{
    // check if this is actually a .desktop file
    bool starts = d->df->desktopGroup().exists();

    // check the hidden field
    starts = starts && !d->df->desktopGroup().readEntry("Hidden", false);

    if (!environment.isEmpty()) {
        starts = starts && checkAllowedEnvironment(environment);
    }

    if (check & CheckCommand) {
        starts = starts && d->df->tryExec();
    }

    if (check & CheckCondition) {
        starts = starts && checkStartCondition();
    }

    return starts;
}

bool PlasmaAutostart::checkStartCondition() const
{
    return PlasmaAutostart::isStartConditionMet(d->df->desktopGroup().readEntry("X-KDE-autostart-condition"));
}

bool PlasmaAutostart::isStartConditionMet(const QString &condition)
{
    if (condition.isEmpty()) {
        return true;
    }

    const QStringList list = condition.split(QLatin1Char(':'));
    if (list.count() < 4) {
        return true;
    }

    if (list[0].isEmpty() || list[2].isEmpty()) {
        return true;
    }

    KConfig config(list[0], KConfig::NoGlobals);
    KConfigGroup cg(&config, list[1]);

    const bool defaultValue = (list[3].toLower() == QLatin1String("true"));
    return cg.readEntry(list[2], defaultValue);
}

bool PlasmaAutostart::checkAllowedEnvironment(const QString &environment) const
{
    const QStringList allowed = allowedEnvironments();
    if (!allowed.isEmpty()) {
        return allowed.contains(environment);
    }

    const QStringList excluded = excludedEnvironments();
    if (!excluded.isEmpty()) {
        return !excluded.contains(environment);
    }

    return true;
}

QString PlasmaAutostart::command() const
{
    return d->df->desktopGroup().readEntry("Exec", QString());
}

void PlasmaAutostart::setCommand(const QString &command)
{
    if (d->df->desktopGroup().readEntry("Exec", QString()) == command) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeEntry("Exec", command);
}

QString PlasmaAutostart::visibleName() const
{
    return d->df->readName();
}

void PlasmaAutostart::setVisibleName(const QString &name)
{
    if (d->df->desktopGroup().readEntry("Name", QString()) == name) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeEntry("Name", name);
}

bool PlasmaAutostart::isServiceRegistered(const QString &entryName)
{
    const QString localDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String("/autostart/");
    return QFile::exists(localDir + entryName + QLatin1String(".desktop"));
}

QString PlasmaAutostart::commandToCheck() const
{
    return d->df->desktopGroup().readPathEntry("TryExec", QString());
}

void PlasmaAutostart::setCommandToCheck(const QString &exec)
{
    if (d->df->desktopGroup().readEntry("TryExec", QString()) == exec) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writePathEntry("TryExec", exec);
}

// do not specialize the readEntry template -
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=100911
static PlasmaAutostart::StartPhase readEntry(const KConfigGroup &group, const char *key, PlasmaAutostart::StartPhase aDefault)
{
    const QByteArray data = group.readEntry(key, QByteArray());

    if (data.isNull()) {
        return aDefault;
    }

    if (data == "0" || data == "BaseDesktop") {
        return PlasmaAutostart::BaseDesktop;
    } else if (data == "1" || data == "DesktopServices") {
        return PlasmaAutostart::DesktopServices;
    } else if (data == "2" || data == "Applications") {
        return PlasmaAutostart::Applications;
    }

    return aDefault;
}

PlasmaAutostart::StartPhase PlasmaAutostart::startPhase() const
{
    return readEntry(d->df->desktopGroup(), "X-KDE-autostart-phase", Applications);
}

void PlasmaAutostart::setStartPhase(PlasmaAutostart::StartPhase phase)
{
    QString data = QStringLiteral("Applications");

    switch (phase) {
    case BaseDesktop:
        data = QStringLiteral("BaseDesktop");
        break;
    case DesktopServices:
        data = QStringLiteral("DesktopServices");
        break;
    case Applications: // This is the default
        break;
    }

    if (d->df->desktopGroup().readEntry("X-KDE-autostart-phase", QString()) == data) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeEntry("X-KDE-autostart-phase", data);
}

QStringList PlasmaAutostart::allowedEnvironments() const
{
    return d->df->desktopGroup().readXdgListEntry("OnlyShowIn");
}

void PlasmaAutostart::setAllowedEnvironments(const QStringList &environments)
{
    if (d->df->desktopGroup().readEntry("OnlyShowIn", QStringList()) == environments) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeXdgListEntry("OnlyShowIn", environments);
}

void PlasmaAutostart::addToAllowedEnvironments(const QString &environment)
{
    QStringList envs = allowedEnvironments();

    if (envs.contains(environment)) {
        return;
    }

    envs.append(environment);
    setAllowedEnvironments(envs);
}

void PlasmaAutostart::removeFromAllowedEnvironments(const QString &environment)
{
    QStringList envs = allowedEnvironments();
    int index = envs.indexOf(environment);

    if (index < 0) {
        return;
    }

    envs.removeAt(index);
    setAllowedEnvironments(envs);
}

QStringList PlasmaAutostart::excludedEnvironments() const
{
    return d->df->desktopGroup().readXdgListEntry("NotShowIn");
}

void PlasmaAutostart::setExcludedEnvironments(const QStringList &environments)
{
    if (d->df->desktopGroup().readEntry("NotShowIn", QStringList()) == environments) {
        return;
    }

    d->copyIfNeeded();
    d->df->desktopGroup().writeXdgListEntry("NotShowIn", environments);
}

void PlasmaAutostart::addToExcludedEnvironments(const QString &environment)
{
    QStringList envs = excludedEnvironments();

    if (envs.contains(environment)) {
        return;
    }

    envs.append(environment);
    setExcludedEnvironments(envs);
}

void PlasmaAutostart::removeFromExcludedEnvironments(const QString &environment)
{
    QStringList envs = excludedEnvironments();
    int index = envs.indexOf(environment);

    if (index < 0) {
        return;
    }

    envs.removeAt(index);
    setExcludedEnvironments(envs);
}

QString PlasmaAutostart::startAfter() const
{
    return d->df->desktopGroup().readEntry("X-KDE-autostart-after");
}
