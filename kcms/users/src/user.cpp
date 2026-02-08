/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "user.h"
#include "kcmusers_debug.h"
#include "user_interface.h"
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KWallet>
#include <QDir>
#include <QImage>
#include <QImageWriter>
#include <config-workspace.h>
#include <kdisplaymanager.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_CRYPT_GENSALT
#include <crypt.h>
#endif

using namespace Qt::StringLiterals;

static const QString CONFIGNAME = QStringLiteral("kcmusers");

User::User(QObject *parent)
    : QObject(parent)
{
    const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    QUrl bestLocationUrl;
    if (!picturesLocations.isEmpty()) {
        bestLocationUrl = QUrl::fromLocalFile(picturesLocations[0]);
    } else {
        bestLocationUrl = QUrl::fromLocalFile(QDir::homePath());
    }

    m_lastFileDialogLocation =
        KSharedConfig::openStateConfig(CONFIGNAME)->group(QStringLiteral("FileDialog")).readEntry(QStringLiteral("LastUsedUrl"), bestLocationUrl);
}

qulonglong User::uid() const
{
    return mUid;
}

void User::setUid(qulonglong value)
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

    if (mFaceCrop) {
        QImage face(value.toLocalFile().remove("file://"_L1));
        mFaceFile = std::make_unique<QTemporaryFile>();
        if (mFaceFile->open()) {
            face = face.copy(mFaceCrop.value());
            face.save(mFaceFile.get(), "PNG");
            mFace = QUrl("file://"_L1 + mFaceFile->fileName());
        } else {
            mError = i18nc("@info", "Failed to crop image: %1", mFaceFile->errorString());
        }
        mFaceCrop.reset(); // crop was applied. reset it to nullopt again so the next face gets its own crop (or none)
    }

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
    if (!m_dbusIface.isNull()) {
        delete m_dbusIface;
    }
    m_dbusIface = new OrgFreedesktopAccountsUserInterface(QStringLiteral("org.freedesktop.Accounts"), path.path(), QDBusConnection::systemBus(), this);
    m_dbusIface->setInteractiveAuthorizationAllowed(true);

    if (m_dbusIface->systemAccount()) {
        return;
    }

    mPath = path;

    connect(m_dbusIface, &OrgFreedesktopAccountsUserInterface::Changed, this, &User::loadData);

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
    const auto localIconFile = QUrl::fromLocalFile(m_dbusIface->iconFile());
    if (mFace != localIconFile) {
        mFace = localIconFile;
        mOriginalFace = mFace;
        mFaceValid = QFileInfo::exists(mFace.path());
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

    mIsCurrentUser = (mUid == getuid());

    mOriginalLoggedIn = mLoggedIn;

    SessList sessions;
    KDisplayManager displayManager;
    displayManager.localSessions(sessions);
    for (auto s : sessions) {
        if (s.user == mOriginalName)
            mLoggedIn = true;
    }

    if (mOriginalLoggedIn != mLoggedIn) {
        userDataChanged = true;
        Q_EMIT loggedInChanged();
    }

    if (userDataChanged) {
        Q_EMIT dataChanged();
    }
}

#if !(HAVE_CRYPT_GENSALT)
static QLatin1Char saltCharacter()
{
    static constexpr quint32 letterCount = 64;
    static constexpr const char saltCharacters[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "./0123456789"; // and trailing NUL
    static_assert(sizeof(saltCharacters) == (letterCount + 1), // 64 letters and trailing NUL
                  "Salt-chars array is not exactly 64 letters long");

    const quint32 index = QRandomGenerator::system()->bounded(0u, letterCount);

    return QLatin1Char(saltCharacters[index]);
}
#endif

static QString saltPassword(const QString &plain)
{
#if HAVE_CRYPT_GENSALT
    QString salt = QString::fromLocal8Bit(crypt_gensalt(NULL, 0, NULL, 0));
#else
    QString salt;

    salt.append(u"$6$");

    for (auto i = 0; i < 16; i++) {
        salt.append(saltCharacter());
    }

    salt.append(u'$');
#endif

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

    auto message = m_dbusIface->SetPassword(saltPassword(password), QString());
    message.waitForFinished();

    // Not an error or invalid message
    if (message.isValid()) {
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
                                opt(mOriginalFace != mFace, mFace.toString(QUrl::PreferLocalFile).remove("file://"_L1)),
                                opt(mOriginalAdministrator != mAdministrator, mAdministrator ? 1 : 0),
                                mError);
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
            case UserApplyJob::Error::UserFacing:
                Q_EMIT applyError(job->errorText());
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
    return wallets.contains(u"kdewallet");
}
void User::changeWalletPassword()
{
    KWallet::Wallet::changePassword(QStringLiteral("kdewallet"), 1);
}

bool User::isCurrentUser() const
{
    return mIsCurrentUser;
}

bool User::loggedIn() const
{
    return mLoggedIn;
}

void User::setFaceCrop(const QRect &rect)
{
    mFaceCrop = rect;
}

QRect User::faceCrop() const
{
    return mFaceCrop.value_or(QRect());
}

QUrl User::lastFileDialogLocation() const
{
    return m_lastFileDialogLocation;
}

void User::setLastFileDialogLocation(QUrl &url)
{
    if (m_lastFileDialogLocation == url) {
        return;
    }

    m_lastFileDialogLocation = url;
    KSharedConfig::openStateConfig(CONFIGNAME)->group(QStringLiteral("FileDialog")).writeEntry(QStringLiteral("LastUsedUrl"), m_lastFileDialogLocation);
    Q_EMIT lastFileDialogLocationChanged();
}

UserApplyJob::UserApplyJob(QPointer<OrgFreedesktopAccountsUserInterface> dbusIface,
                           std::optional<QString> name,
                           std::optional<QString> email,
                           std::optional<QString> realname,
                           std::optional<QString> icon,
                           std::optional<int> type,
                           std::optional<QString> error)
    : KJob()
    , m_name(name)
    , m_email(email)
    , m_realname(realname)
    , m_icon(icon)
    , m_type(type)
    , m_dbusIface(dbusIface)
    , m_error(error)
{
}

void UserApplyJob::start()
{
    // With our UI the user expects the as a single transaction, but the accountsservice API does not provide that
    // When one of the writes fails, e.g. because the user cancelled the authentication dialog then none of the values should be applied
    // Not all calls trigger an authentication dialog, e.g. SetRealName for the current user does not but SetAccountType does
    // Therefore make a blocking call to SetAccountType first to trigger the auth dialog. If the user declines don't attempt to write anything else
    // This avoids settings any data when the user thinks they aborted the transaction, see https://bugs.kde.org/show_bug.cgi?id=425036
    // Subsequent calls do not trigger the auth dialog again

    if (m_error) {
        setErrorText(m_error.value());
        KJob::setError(static_cast<int>(Error::UserFacing));
        emitResult();
        return;
    }

    if (m_type.has_value()) {
        auto setAccount = m_dbusIface->SetAccountType(*m_type);
        setAccount.waitForFinished();
        if (setAccount.isError()) {
            setError(setAccount.error());
            qCWarning(KCMUSERS) << setAccount.error().name() << setAccount.error().message();
            emitResult();
            return;
        }
    }

    using UserPropertySetter = QDBusPendingReply<> (OrgFreedesktopAccountsUserInterface::*)(const QString &name);
    const std::array<std::pair<std::optional<QString>, UserPropertySetter>, 3> set = {{
        {m_name, &OrgFreedesktopAccountsUserInterface::SetUserName},
        {m_email, &OrgFreedesktopAccountsUserInterface::SetEmail},
        {m_realname, &OrgFreedesktopAccountsUserInterface::SetRealName},
    }};
    for (auto const &[field, setter] : set) {
        if (!field.has_value()) {
            continue;
        }

        auto resp = (m_dbusIface->*setter)(field.value());
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

        auto resp = m_dbusIface->SetIconFile(file.fileName());

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

#include "moc_user.cpp"
