/*
    localegeneratorubuntu.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "localegeneratorbase.h"
#include <QProcess>

class LocaleGeneratorUbuntu : public LocaleGeneratorBase
{
    Q_OBJECT
public:
    using LocaleGeneratorBase::LocaleGeneratorBase;
    Q_INVOKABLE void localesGenerate(const QStringList &list) override;

private:
    QProcess *m_proc{nullptr};
    QStringList m_packageIDs;

    void ubuntuInstall(const QStringList &locales);
private Q_SLOTS:
    void ubuntuLangCheck(int statusCode, QProcess::ExitStatus exitStatus);
};
