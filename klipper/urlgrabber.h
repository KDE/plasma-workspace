/*
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QStringList>

#include <KSharedConfig>

#include "klipper_export.h"

class KLIPPER_EXPORT ClipCommand
{
public:
    /**
     * What to do with output of command
     */
    enum Output {
        IGNORE, // Discard output
        REPLACE, // Replace clipboard entry with output
        ADD, // Add output as new clipboard element
    };

    explicit ClipCommand();
    explicit ClipCommand(const QString &_command,
                         const QString &_description,
                         bool enabled = true,
                         const QString &_icon = QString(),
                         Output _output = IGNORE,
                         const QString &serviceStorageId = QString());

    QString command;
    QString description;
    bool isEnabled = true;
    QString icon;
    Output output = IGNORE;
    // If this is set, it's an app to handle a mimetype, and will be launched normally using KRun.
    // StorageId is used instead of KService::Ptr, because the latter disallows operator=.
    QString serviceStorageId;
};
Q_DECLARE_METATYPE(ClipCommand)

/**
 * Represents one configured action. An action consists of one regular
 * expression, an (optional) description and a list of ClipCommands
 * (a command to be executed, a description and an enabled/disabled flag).
 */
class KLIPPER_EXPORT ClipAction
{
public:
    explicit ClipAction(const QString &regExp = QString(), const QString &description = QString(), bool automagic = true);
    explicit ClipAction(const KSharedConfigPtr &config, const QString &group);
    ~ClipAction();

    void addCommand(const ClipCommand &cmd);

    QString actionRegexPattern;
    QStringList actionCapturedTexts;
    QString description;
    QList<ClipCommand> commands;
    bool automatic = false;
};

using ActionList = QList<ClipAction>;

namespace KLIPPER_EXPORT URLGrabber
{
ActionList loadActions();
void saveActions(const ActionList &actions);
}
