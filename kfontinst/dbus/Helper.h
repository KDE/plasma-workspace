#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2010 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KAuth/ActionReply>
#include <QObject>
#include <QSet>

using namespace KAuth;

namespace KFI
{
class Helper : public QObject
{
    Q_OBJECT

public:
    Helper();
    ~Helper() override;

public Q_SLOTS:

    ActionReply manage(const QVariantMap &args);

private:
    int install(const QVariantMap &args);
    int uninstall(const QVariantMap &args);
    int move(const QVariantMap &args);
    int toggle(const QVariantMap &args);
    int removeFile(const QVariantMap &args);
    int reconfigure();
    int saveDisabled();
    int checkWriteAction(const QStringList &files);
};

}
