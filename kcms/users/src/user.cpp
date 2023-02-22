/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "user.h"
#include "kcmusers_debug.h"
#include "user_interface.h"
#include <KLocalizedString>
#include <KWallet>
#include <QImage>
#include <QtConcurrent>
#include <sys/types.h>
#include <unistd.h>

User::User(QObject *parent)
    : QObject(parent)
{
}

int User::uid() const
{
    return mUid;
}

void User::setUid(int value)
{
    if (mUid == value) {
        return;
    }
    mUid = value;
    Q_EMIT uidChanged();
}

QString User::name() const
{
    return mName;
}

void User::setName(const QString &value)
{
    if (mName == value) {
        return;
    }
    mName = value;
    Q_EMIT nameChanged();
    Q_EMIT displayNamesChanged();
}

QString User::realName() const
{
    return mRealName;
}

void User::setRealName(const QString &value)
{
    if (mRealName == value) {
        return;
    }
    mRealName = value;
    Q_EMIT realNameChanged();
    Q_EMIT displayNamesChanged();
}

QString User::displayPrimaryName() const
{
    return !mRealName.isEmpty() ? mRealName : mName;
}

QString User::displaySecondaryName() const
{
    return !mRealName.isEmpty() ? mName : QString();
}

QString User::email() const
{
    return mEmail;
}

void User::setEmail(const QString &value)
{
    if (mEmail == value) {
        return;
    }
    mEmail = value;
    Q_EMIT emailChanged();
}

QUrl User::face() const
{
    return mFace;
}

bool User::faceValid() const
{
    return mFaceValid;
}

void User::setFace(const QUrl &value)
{
    if (mFace == value) {
        return;
    }
    mFace = value;
    mFaceValid = QFile::exists(value.path());
    Q_EMIT faceValidChanged();
    Q_EMIT faceChanged();
}

bool User::administrator() const
{
    return mAdministrator;
}
void User::setAdministrator(bool value)
{
    if (mAdministrator == value) {
        return;
    }
    mAdministrator = value;
    Q_EMIT administratorChanged();
}

void User::setPath(const QDBusObjectPath &path)
{
    if (!m_dbusIface.isNull())
        delete m_dbusIface;
    m_dbusIface = new OrgFreedesktopAccountsUserInterface(QStringLiteral("org.freedesktop.Accounts"), path.path(), QDBusConnection::systemBus(), this);

    if (m_dbusIface->systemAccount()) {
        return;
    }

    mPath = path;

    connect(m_dbusIface, &OrgFreedesktopAccountsUserInterface::Changed, [=]() {
        loadData();
    });

    loadData();
}

void User::loadData()
{
    bool userDataChanged = false;
    if (mUid != m_dbusIface->uid()) {
        mUid = m_dbusIface->uid();
        mOriginalUid = mUid;
        userDataChanged = true;
        Q_EMIT uidChanged();
    }
    if (mName != m_dbusIface->userName()) {
        mName = m_dbusIface->userName();
        mOriginalName = mName;
        userDataChanged = true;
        Q_EMIT nameChanged();
    }
    if (mFace != QUrl(m_dbusIface->iconFile())) {
        mFace = QUrl(m_dbusIface->iconFile());
        mOriginalFace = mFace;
        mFaceValid = QFileInfo::exists(mFace.toString());
        mOriginalFaceValid = mFaceValid;
        userDataChanged = true;
        Q_EMIT faceChanged();
        Q_EMIT faceValidChanged();
    }
    if (mRealName != m_dbusIface->realName()) {
        mRealName = m_dbusIface->realName();
        mOriginalRealName = mRealName;
        userDataChanged = true;
        Q_EMIT realNameChanged();
    }
    if (mEmail != m_dbusIface->email()) {
        mEmail = m_dbusIface->email();
        mOriginalEmail = mEmail;
        userDataChanged = true;
        Q_EMIT emailChanged();
    }
    const auto administrator = (m_dbusIface->accountType() == 1);
    if (mAdministrator != administrator) {
        mAdministrator = administrator;
        mOriginalAdministrator = mAdministrator;
        userDataChanged = true;
        Q_EMIT administratorChanged();
    }
    const auto loggedIn = (mUid == getuid());
    if (mLoggedIn != loggedIn) {
        mLoggedIn = loggedIn;
        mOriginalLoggedIn = mLoggedIn;
        userDataChanged = true;
    }
    if (userDataChanged) {
        Q_EMIT dataChanged();
    }
}

static char saltCharacter()
{
    static constexpr const quint32 letterCount = 64;
    static const char saltCharacters[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "./0123456789"; // and trailing NUL
    static_assert(sizeof(saltCharacters) == (letterCount + 1), // 64 letters and trailing NUL
                  "Salt-chars array is not exactly 64 letters long");

    const quint32 index = QRandomGenerator::system()->bounded(0u, letterCount);

    return saltCharacters[index];
}

static QString saltPassword(const QString &plain)
{
    QString salt;

    salt.append("$6$");

    for (auto i = 0; i < 16; i++) {
        salt.append(saltCharacter());
    }

    salt.append("$");

    auto stdStrPlain = plain.toStdString();
    auto cStrPlain = stdStrPlain.c_str();
    auto stdStrSalt = salt.toStdString();
    auto cStrSalt = stdStrSalt.c_str();

    auto salted = crypt(cStrPlain, cStrSalt);

    return QString::fromUtf8(salted);
}

void User::setPassword(const QString &password)
{
    // Blocking because we need to wait for the password to be changed before we
    // can ask the user about also possibly changing their KWallet password

    auto mc = QDBusMessage::createMethodCall(m_dbusIface->service(), m_dbusIface->path(), m_dbusIface->interface(), "SetPassword");
    mc.setArguments({saltPassword(password), QString()});
    mc.setInteractiveAuthorizationAllowed(true);
    auto message = QDBusConnection::systemBus().call(mc);

    // Not an error or invalid message
    if (message.type() == QDBusMessage::ReplyMessage) {
        Q_EMIT passwordSuccessfullyChanged();
    }
}

QDBusObjectPath User::path() const
{
    return mPath;
}

void User::apply()
{
    const auto opt = [](bool cond, auto v) {
        if (cond) {
            return std::optional<decltype(v)>(v);
        }
        return std::optional<decltype(v)>();
    };
    auto job = new UserApplyJob(m_dbusIface,
                                opt(mOriginalName != mName, mName),
                                opt(mOriginalEmail != mEmail, mEmail),
                                opt(mOriginalRealName != mRealName, mRealName),
                                opt(mOriginalFace != mFace, mFace.toString().replace("file://", "")),
                                opt(mOriginalAdministrator != mAdministrator, mAdministrator ? 1 : 0));
    connect(
        job,
        &UserApplyJob::result,
        this,
        [this, job] {
            switch (static_cast<UserApplyJob::Error>(job->error())) {
            case UserApplyJob::Error::PermissionDenied:
                loadData(); // Reload the old data to avoid half transactions
                Q_EMIT applyError(i18n("Could not get permission to save user %1", mName));
                break;
            case UserApplyJob::Error::Failed:
            case UserApplyJob::Error::Unknown:
                loadData(); // Reload the old data to avoid half transactions
                Q_EMIT applyError(i18n("There was an error while saving changes"));
                break;
            case UserApplyJob::Error::NoError:; // Do nothing
            }
        },
        Qt::QueuedConnection /* result emission must be async, in the case of failure we'll manipulate states */);
    job->start();
}

bool User::usesDefaultWallet()
{
    const QStringList wallets = KWallet::Wallet::walletList();
    return wallets.contains(QStringLiteral("kdewallet"));
}
void User::changeWalletPassword()
{
    KWallet::Wallet::changePassword(QStringLiteral("kdewallet"), 1);
}

bool User::loggedIn() const
{
    return mLoggedIn;
}

UserApplyJob::UserApplyJob(QPointer<OrgFreedesktopAccountsUserInterface> dbusIface,
                           std::optional<QString> name,
                           std::optional<QString> email,
                           std::optional<QString> realname,
                           std::optional<QString> icon,
                           std::optional<int> type)
    : KJob()
    , m_name(name)
    , m_email(email)
    , m_realname(realname)
    , m_icon(icon)
    , m_type(type)
    , m_dbusIface(dbusIface)
{
}

// Work around QTBUG-100458
inline auto asyncCall(OrgFreedesktopAccountsUserInterface *ptr, const QString &method, const QVariantList &arguments)
{
    auto mc = QDBusMessage::createMethodCall(ptr->service(), ptr->path(), ptr->interface(), method);
    mc.setArguments(arguments);
    mc.setInteractiveAuthorizationAllowed(true);
    return QDBusConnection::systemBus().asyncCall(mc);
}

void UserApplyJob::start()
{
    // With our UI the user expects the as a single transaction, but the accountsservice API does not provide that
    // When one of the writes fails, e.g. because the user cancelled the authentication dialog then none of the values should be applied
    // Not all calls trigger an authentication dialog, e.g. SetRealName for the current user does not but SetAccountType does
    // Therefore make a blocking call to SetAccountType first to trigger the auth dialog. If the user declines don't attempt to write anything else
    // This avoids settings any data when the user thinks they aborted the transaction, see https://bugs.kde.org/show_bug.cgi?id=425036
    // Subsequent calls do not trigger the auth dialog again

    if (m_type.has_value()) {
        auto setAccount = asyncCall(m_dbusIface, "SetAccountType", {*m_type});
        setAccount.waitForFinished();
        if (setAccount.isError()) {
            setError(setAccount.error());
            qCWarning(KCMUSERS) << setAccount.error().name() << setAccount.error().message();
            emitResult();
            return;
        }
    }

    const std::multimap<std::optional<QString>, QString> set = {
        {m_name, "SetUserName"},
        {m_email, "SetEmail"},
        {m_realname, "SetRealName"},
    };
    for (auto const &x : set) {
        if (!x.first.has_value())
            continue;

        auto resp = asyncCall(m_dbusIface, x.second, {*x.first});
        resp.waitForFinished();
        if (resp.isError()) {
            setError(resp.error());
            qCWarning(KCMUSERS) << resp.error().name() << resp.error().message();
            emitResult();
            return;
        }
    }

    // Icon is special, since we want to resize it.
    if (m_icon.has_value()) {
        QImage icon(*m_icon);
        // 256dp square is plenty big for an avatar and will definitely be smaller than 1MB
        QImage scaled = icon.scaled(QSize(256, 256), Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QTemporaryFile file;
        if (!file.open()) {
            setErrorText(i18n("Failed to resize image: opening temp file failed"));
            qCWarning(KCMUSERS) << "Failed to resize image: opening temp file failed";
            KJob::setError(static_cast<int>(Error::UserFacing));
            emitResult();
            return;
        }

        if (!scaled.save(&file, "png")) {
            setErrorText(i18n("Failed to resize image: writing to temp file failed"));
            qCWarning(KCMUSERS) << "Failed to resize image: writing to temp file failed";
            KJob::setError(static_cast<int>(Error::UserFacing));
            emitResult();
            return;
        }

        file.close();

        auto resp = asyncCall(m_dbusIface, "SetIconFile", {file.fileName()});

        resp.waitForFinished();
        if (resp.isError()) {
            setError(resp.error());
            qCWarning(KCMUSERS) << resp.error().name() << resp.error().message();
            emitResult();
            return;
        }
    }

    emitResult();
}

void UserApplyJob::setError(const QDBusError &error)
{
    setErrorText(error.message());
    if (error.name() == QLatin1String("org.freedesktop.Accounts.Error.Failed")) {
        KJob::setError(static_cast<int>(Error::Failed));
    } else if (error.name() == QLatin1String("org.freedesktop.Accounts.Error.PermissionDenied")) {
        KJob::setError(static_cast<int>(Error::PermissionDenied));
    } else {
        KJob::setError(static_cast<int>(Error::Unknown));
    }
}
