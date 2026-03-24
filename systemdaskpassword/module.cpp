/*
    SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "module.h"
#include "askpasswordagent.h"

#include <KPluginFactory>

#include <QStandardPaths>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(SystemdAskPasswordModule, "systemdaskpassword.json")

SystemdAskPasswordModule::SystemdAskPasswordModule(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    new AskPasswordAgent(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + u"/systemd/ask-password"_s, this);
}

#include "module.moc"
