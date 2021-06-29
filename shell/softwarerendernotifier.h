/*
    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KStatusNotifierItem>

/**
 * Responsible for showing an SNI if the software renderer is used
 * to allow the a user to open the KCM
 */

class SoftwareRendererNotifier : public KStatusNotifierItem
{
    Q_OBJECT
public:
    // only exposed as void static constructor as internally it is self memory managing
    static void notifyIfRelevant();

private:
    SoftwareRendererNotifier(QObject *parent = nullptr);
    ~SoftwareRendererNotifier();
};
