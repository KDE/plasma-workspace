/*
    localegeneratorbase.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KLocalizedString>
#include <QObject>

class LocaleGeneratorBase : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    Q_INVOKABLE virtual void localesGenerate(const QStringList &list);

Q_SIGNALS:
    void success();
    void needsFont();
    void userHasToGenerateManually(const QString &reason);

protected:
    static inline QString defaultManuallyGenerateMessage()
    {
        return i18nc("@info:warning",
                     "Locale has been configured, but this KCM currently "
                     "doesn't support auto locale generation on your system, "
                     "please refer to your distribution's manual to install fonts and generate locales");
    }
};
