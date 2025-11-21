/*
    SPDX-FileCopyrightText: 2000, 2001, 2002 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "urlgrabber.h"

#include <QIcon>

#include <KConfigGroup>

#include "klippersettings.h"

using namespace Qt::StringLiterals;

namespace URLGrabber
{

ActionList loadActions()
{
    ActionList actions;
    const KSharedConfig::Ptr config = KSharedConfig::openConfig();
    const KConfigGroup cg(config, QStringLiteral("General"));
    for (int i = 0, num = cg.readEntry("Number of Actions", 0); i < num; i++) {
        const QString group = QStringLiteral("Action_%1").arg(i);
        actions.emplace_back(config, group);
    }
    return actions;
}

void saveActions(const ActionList &actions)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cg(config, u"General"_s);
    cg.writeEntry("Number of Actions", actions.size());

    for (int i = 0, count = actions.size(); i < count; ++i) {
        const ClipAction &action = actions[i];
        const QString groupName = QStringLiteral("Action_%1").arg(i);
        KConfigGroup cg(config, groupName);
        cg.writeEntry("Description", action.description);
        cg.writeEntry("Regexp", action.actionRegexPattern);
        cg.writeEntry("Number of commands", action.commands.size());
        cg.writeEntry("Automatic", action.automatic);

        // now iterate over all commands of this action
        for (int j = 0, listSize = action.commands.size(); j < listSize; ++j) {
            QString _group = groupName + QStringLiteral("/Command_%1");
            KConfigGroup cg(config, _group.arg(j));
            const ClipCommand &cmd = action.commands[j];
            cg.writePathEntry("Commandline", cmd.command);
            cg.writeEntry("Description", cmd.description);
            cg.writeEntry("Enabled", cmd.isEnabled);
            cg.writeEntry("Icon", cmd.icon);
            cg.writeEntry("Output", std::to_underlying(cmd.output));
        }
    }

    Q_EMIT KlipperSettings::self()->ActionListChanged();
}
}

ClipCommand::ClipCommand()
{
}

ClipCommand::ClipCommand(const QString &_command,
                         const QString &_description,
                         bool _isEnabled,
                         const QString &_icon,
                         Output _output,
                         const QString &_serviceStorageId)
    : command(_command)
    , description(_description)
    , isEnabled(_isEnabled)
    , output(_output)
    , serviceStorageId(_serviceStorageId)
{
    if (!_icon.isEmpty())
        icon = _icon;
    else {
        // try to find suitable icon
        QString appName = command.section(QLatin1Char(' '), 0, 0);
        if (!appName.isEmpty()) {
            if (QIcon::hasThemeIcon(appName))
                icon = appName;
            else
                icon.clear();
        }
    }
}

ClipAction::ClipAction(const QString &regExp, const QString &description, bool automatic)
    : actionRegexPattern(regExp)
    , description(description)
    , automatic(automatic)
{
}

ClipAction::ClipAction(const KSharedConfigPtr &config, const QString &group)
    : actionRegexPattern(config->group(group).readEntry("Regexp"))
    , description(config->group(group).readEntry("Description"))
    , automatic(config->group(group).readEntry("Automatic", QVariant(true)).toBool())
{
    KConfigGroup cg(config, group);
    // read the commands
    for (int i = 0, listSize = cg.readEntry("Number of commands", 0); i < listSize; i++) {
        QString _group = group + QStringLiteral("/Command_%1");
        KConfigGroup _cg(config, _group.arg(i));

        addCommand(ClipCommand(_cg.readPathEntry("Commandline", QString()),
                               _cg.readEntry("Description"), // i18n'ed
                               _cg.readEntry("Enabled", false),
                               _cg.readEntry("Icon"),
                               static_cast<ClipCommand::Output>(_cg.readEntry("Output", QVariant(ClipCommand::IGNORE)).toInt())));
    }
}

ClipAction::~ClipAction() = default;

void ClipAction::addCommand(const ClipCommand &cmd)
{
    if (cmd.command.isEmpty() && cmd.serviceStorageId.isEmpty()) {
        return;
    }
    commands.append(cmd);
}

#include "moc_urlgrabber.cpp"
