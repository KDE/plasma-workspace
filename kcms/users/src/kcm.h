/*
    SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "fingerprintmodel.h"
#include "usermodel.h"
#include <KQuickConfigModule>
#include <qqmlintegration.h>

class OrgFreedesktopAccountsInterface;

class KCMUser : public KQuickConfigModule
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(UserModel *userModel MEMBER m_model CONSTANT)
    Q_PROPERTY(QStringList avatarFiles MEMBER m_avatarFiles CONSTANT)
    Q_PROPERTY(FingerprintModel *fingerprintModel MEMBER m_fingerprintModel CONSTANT)

private:
    OrgFreedesktopAccountsInterface *const m_dbusInterface;
    UserModel *const m_model;
    QStringList m_avatarFiles;
    FingerprintModel *const m_fingerprintModel;

public:
    KCMUser(QObject *parent, const KPluginMetaData &data);
    ~KCMUser() override;

    Q_SCRIPTABLE bool createUser(const QString &name, const QString &realName, const QString &password, bool admin);
    Q_SCRIPTABLE bool deleteUser(qint64 index, bool deleteHome);
    Q_SCRIPTABLE QUrl recolorSVG(const QUrl &url, const QColor &color);
    // Grab the initials of a string
    Q_SCRIPTABLE QString initializeString(const QString &stringToGrabInitialsOf);
    Q_SCRIPTABLE QString plonkImageInTempfile(const QImage &image);

Q_SIGNALS:
    Q_SCRIPTABLE void apply();
    Q_SCRIPTABLE void reset();

public Q_SLOTS:
    void save() override;
    void load() override;
};
