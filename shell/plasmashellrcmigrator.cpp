/*
    SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "plasmashellrcmigrator.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QStringList>

void copyGroup(KConfigGroup &sourceRoot, KConfigGroup &targetRoot, const QString &groupName)
{
    if (!sourceRoot.hasGroup(groupName)) {
        return;
    }

    KConfigGroup sourceGroup(&sourceRoot, groupName);
    KConfigGroup targetGroup(&targetRoot, groupName);
    sourceGroup.copyTo(&targetGroup);
}

QString shellConfigName(const QString &shellPackage)
{
    return u"plasma-" + shellPackage + u"-shellrc";
}

QStringList shellPackagesWithLayout()
{
    const QDir configDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation));
    const QStringList appletConfigFiles = configDir.entryList({QStringLiteral("plasma-*-appletsrc")}, QDir::Files | QDir::Readable, QDir::Name);
    QStringList shellPackages;
    shellPackages.reserve(appletConfigFiles.size());

    for (const QString &fileName : appletConfigFiles) {
        QString shellPackage = fileName;
        shellPackage.remove(0, QStringLiteral("plasma-").size());
        shellPackage.chop(QStringLiteral("-appletsrc").size());

        if (!shellPackages.contains(shellPackage)) {
            shellPackages.append(shellPackage);
        }
    }

    return shellPackages;
}

bool addMigrationComment()
{
    QFile file(QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)).filePath(QStringLiteral("plasmashellrc")));
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return false;
    }
    const QByteArray oldContents = file.readAll();
    file.seek(0);
    file.write("# Plasma 6.8 migration: settings formerly read from this file have been copied to plasmarc and plasma-*-shellrc.\n");
    file.write(oldContents);
    return file.error() == QFile::NoError;
}

void migratePlasmashellrc()
{
    const KSharedConfigPtr plasmaConfig = KSharedConfig::openConfig(QStringLiteral("plasmarc"), KConfig::SimpleConfig);
    KConfigGroup migrationGroup(plasmaConfig, QStringLiteral("Migration"));
    if (migrationGroup.readEntry("plasmashellrcToShellrc", false)) {
        return;
    }

    const KSharedConfigPtr oldConfig = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"), KConfig::SimpleConfig);
    KConfigGroup oldRoot = oldConfig->group(QString());
    KConfigGroup oldShellGroup(oldConfig, QStringLiteral("Shell"));
    bool success = true;
    bool migrated = false;

    // Copy ShellPackage=... from ~/.config/plasmashellrc to ~/.config/plasmarc
    if (oldShellGroup.hasKey("ShellPackage")) {
        KConfigGroup newShellGroup(plasmaConfig, QStringLiteral("Shell"));
        newShellGroup.writeEntry("ShellPackage", oldShellGroup.readEntry("ShellPackage", QString()));
        migrated = true;
    }

    QStringList shellGroups = oldConfig->groupList();
    shellGroups.removeAll(QStringLiteral("Shell"));

    // Loop over all new *-shellrc files and copy plasmashellrc config groups over
    // We don't know which setting is for which shell (especially applet configuration), so just copy it to all of the shells
    const QStringList shellPackages = shellPackagesWithLayout();
    for (const QString &shellPackage : shellPackages) {
        const KSharedConfigPtr shellConfig = KSharedConfig::openConfig(shellConfigName(shellPackage), KConfig::SimpleConfig);
        KConfigGroup shellRoot = shellConfig->group(QString());

        for (const QString &group : shellGroups) {
            copyGroup(oldRoot, shellRoot, group);
        }
        success = shellConfig->sync() && success;
    }

    if (!shellPackages.isEmpty() && !shellGroups.isEmpty()) {
        migrated = true;
    }
    if (!migrated || !success || !addMigrationComment()) {
        return;
    }

    migrationGroup.writeEntry("plasmashellrcToShellrc", true);
    plasmaConfig->sync();
}
