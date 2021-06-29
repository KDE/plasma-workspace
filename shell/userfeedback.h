/*
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2020 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ShellCorona;

namespace KUserFeedback
{
class Provider;
}

class UserFeedback : public QObject
{
    Q_OBJECT
public:
    UserFeedback(ShellCorona *corona, QObject *parent);
    ~UserFeedback() override = default;
    QString describeDataSources() const;

private:
    KUserFeedback::Provider *m_provider;
};
