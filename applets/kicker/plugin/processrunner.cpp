/*
    SPDX-FileCopyrightText: 2013 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "processrunner.h"

#include <KProcess>

ProcessRunner::ProcessRunner(QObject *parent)
    : QObject(parent)
{
}

ProcessRunner::~ProcessRunner()
{
}

void ProcessRunner::runMenuEditor()
{
    KProcess::startDetached(QStringLiteral("kmenuedit"));
}
