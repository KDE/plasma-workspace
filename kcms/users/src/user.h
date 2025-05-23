/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <KJob>
#include <QDBusObjectPath>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QTemporaryFile>
#include <QUrl>

#include <optional>

class OrgFreedesktopAccountsUserInterface;
class QDBusError;

class UserApplyJob : public KJob
{
    Q_OBJECT

public:
    UserApplyJob(QPointer<OrgFreedesktopAccountsUserInterface> dbusIface,
                 std::optional<QString> name,
                 std::optional<QString> email,
                 std::optional<QString> realname,
                 std::optional<QString> icon,
                 std::optional<int> type,
                 std::optional<QString> error);
    void start() override;

    enum class Error {
        NoError = 0,
        PermissionDenied,
        Failed,
        Unknown,
        UserFacing,
    };

private:
    void setError(const QDBusError &error);

    std::optional<QString> m_name;
    std::optional<QString> m_email;
    std::optional<QString> m_realname;
    std::optional<QString> m_icon;
    std::optional<int> m_type;
    QPointer<OrgFreedesktopAccountsUserInterface> m_dbusIface;
    std::optional<QString> m_error;
};

class User : public QObject
{
    Q_OBJECT

    Q_PROPERTY(qulonglong uid READ uid WRITE setUid NOTIFY uidChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

    Q_PROPERTY(QString realName READ realName WRITE setRealName NOTIFY realNameChanged)

    // If realName is set, then primary is real name and secondary is username.
    // Otherwise, primary is username and secondary is empty.
    // This is useful for KCM delegates and pages to display "at least something" and "something else or nothing" text.
    Q_PROPERTY(QString displayPrimaryName READ displayPrimaryName NOTIFY displayNamesChanged)
    Q_PROPERTY(QString displaySecondaryName READ displaySecondaryName NOTIFY displayNamesChanged)

    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)

    Q_PROPERTY(QUrl face READ face WRITE setFace NOTIFY faceChanged)

    Q_PROPERTY(QRect faceCrop READ faceCrop WRITE setFaceCrop)

    Q_PROPERTY(bool faceValid READ faceValid NOTIFY faceValidChanged)

    Q_PROPERTY(bool isCurrentUser READ isCurrentUser CONSTANT)

    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loggedInChanged)

    Q_PROPERTY(bool administrator READ administrator WRITE setAdministrator NOTIFY administratorChanged)

    Q_PROPERTY(QUrl lastFileDialogLocation READ lastFileDialogLocation WRITE setLastFileDialogLocation NOTIFY lastFileDialogLocationChanged)

public:
    explicit User(QObject *parent = nullptr);

    qulonglong uid() const;
    void setUid(qulonglong value);

    QString name() const;
    QString realName() const;
    QString displayPrimaryName() const;
    QString displaySecondaryName() const;
    QString email() const;
    QUrl face() const;
    bool faceValid() const;
    bool isCurrentUser() const;
    bool loggedIn() const;
    bool administrator() const;
    QDBusObjectPath path() const;
    QRect faceCrop() const;
    QUrl lastFileDialogLocation() const;

    void setName(const QString &value);
    void setRealName(const QString &value);
    void setEmail(const QString &value);
    void setFace(const QUrl &value);
    void setAdministrator(bool value);
    void setPath(const QDBusObjectPath &path);
    void setFaceCrop(const QRect &rect);
    void setLastFileDialogLocation(QUrl &url);

    void loadData();

public Q_SLOTS:
    Q_SCRIPTABLE void apply();
    Q_SCRIPTABLE bool usesDefaultWallet();
    Q_SCRIPTABLE void changeWalletPassword();
    Q_SCRIPTABLE void setPassword(const QString &value);

Q_SIGNALS:
    void dataChanged();
    void uidChanged();
    void nameChanged();
    void realNameChanged();
    void displayNamesChanged();
    void emailChanged();
    void loggedInChanged();
    void faceChanged();
    void faceValidChanged();
    void administratorChanged();
    void applyError(const QString &errorMessage);
    void passwordSuccessfullyChanged();
    void lastFileDialogLocationChanged();

private:
    qulonglong mUid = 0;
    int mOriginalUid = 0;
    QString mName;
    QString mOriginalName;
    QString mRealName;
    QString mOriginalRealName;
    QString mEmail;
    QString mOriginalEmail;
    QUrl mFace;
    QUrl mOriginalFace;
    std::unique_ptr<QTemporaryFile> mFaceFile;
    std::optional<QRect> mFaceCrop;
    bool mAdministrator = false;
    bool mOriginalAdministrator = false;
    bool mFaceValid = false;
    bool mOriginalFaceValid = false;
    bool mIsCurrentUser = false;
    bool mLoggedIn = false;
    bool mOriginalLoggedIn = false;
    QDBusObjectPath mPath;
    QPointer<OrgFreedesktopAccountsUserInterface> m_dbusIface;
    std::optional<QString> mError;
    QUrl m_lastFileDialogLocation;
};
