/*
    SPDX-FileCopyrightText: 2016-2018 Jan Grulich <jgrulich@redhat.com>
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "kcm.h"

// KDE
#include <KLocalizedString>
#include <KPluginFactory>
// Qt
#include <QApplication>
#include <QFileDialog>
#include <QFontMetrics>
#include <QImage>
#include <QQmlProperty>
#include <QQuickItemGrabResult>
#include <QTemporaryFile>
#include <QTimer>
#include <QtQuick/QQuickItem>

#include "accounts_interface.h"
#include "maskmousearea.h"
#include "user.h"

Q_LOGGING_CATEGORY(kcm_users, "kcm_users")

K_PLUGIN_CLASS_WITH_JSON(KCMUser, "kcm_users.json")

using namespace Qt::StringLiterals;

KCMUser::KCMUser(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
    , m_dbusInterface(new OrgFreedesktopAccountsInterface(QStringLiteral("org.freedesktop.Accounts"),
                                                          QStringLiteral("/org/freedesktop/Accounts"),
                                                          QDBusConnection::systemBus(),
                                                          this))
    , m_model(new UserModel(this))
    , m_fingerprintModel(new FingerprintModel(this))
{
    m_dbusInterface->setInteractiveAuthorizationAllowed(true);

    constexpr const char *uri = "org.kde.plasma.kcm.users";

    qmlRegisterUncreatableType<UserModel>(uri, 1, 0, "UserModel", QStringLiteral("Registered for enum access only"));
    qmlRegisterUncreatableType<User>(uri, 1, 0, "User", QStringLiteral("Use kcm.userModel to access User objects"));
    qmlRegisterType<MaskMouseArea>(uri, 1, 0, "MaskMouseArea");

    constexpr const char *uri_fm = "FingerprintModel";

    qmlRegisterUncreatableType<FprintDevice>(uri_fm, 1, 0, "FprintDevice", QStringLiteral("Only for enum access"));

    setButtons(Apply);
    auto font = QApplication::font("QLabel");
    auto fm = QFontMetrics(font);
    setColumnWidth(fm.capHeight() * 30);

    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("plasma/avatars"), QStandardPaths::LocateDirectory);
    for (const auto &dir : dirs) {
        QDirIterator it(dir, QStringList{QStringLiteral("*.jpg"), QStringLiteral("*.png")}, QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext()) {
            m_avatarFiles << it.next();
        }
    }
}

bool KCMUser::createUser(const QString &name, const QString &realName, const QString &password, bool isAdmin)
{
    QDBusPendingReply<QDBusObjectPath> reply = m_dbusInterface->CreateUser(name, realName, static_cast<qint32>(isAdmin));
    reply.waitForFinished();
    if (reply.isValid()) {
        User *createdUser = new User(this);
        createdUser->setPath(reply.value());
        createdUser->setPassword(password);
        delete createdUser;
        return true;
    }
    return false;
}

bool KCMUser::deleteUser(qint64 id, bool deleteHome)
{
    QDBusPendingReply<> reply = m_dbusInterface->DeleteUser(id, deleteHome);
    reply.waitForFinished();
    if (reply.isError()) {
        return false;
    } else {
        return true;
    }
}

KCMUser::~KCMUser() = default;

void KCMUser::save()
{
    KQuickConfigModule::save();
    Q_EMIT apply();
}

// Grab the initials of a string
QString KCMUser::initializeString(const QString &stringToGrabInitialsOf)
{
    if (stringToGrabInitialsOf.isEmpty()) {
        return {};
    }

    auto normalized = stringToGrabInitialsOf.normalized(QString::NormalizationForm_D);
    if (normalized.contains(u' ')) {
        QStringList split = normalized.split(u' ');
        auto first = split.first();
        auto last = split.last();
        if (first.isEmpty()) {
            return {last.front()};
        }
        if (last.isEmpty()) {
            return {first.front()};
        }
        return QString(first.front()) + last.front();
    } else {
        return {normalized.front()};
    }
}

QString KCMUser::plonkImageInTempfile(const QImage &image)
{
    auto file = new QTemporaryFile(qApp);
    if (file->open()) {
        image.save(file, "PNG");
    }
    return file->fileName();
}

QUrl KCMUser::recolorSVG(const QUrl &url, const QColor &color)
{
    static QMap<QUrl, QString> s_cache;

    if (!s_cache.contains(url)) {
        QFile at(url.toString().sliced(QLatin1String("qrc").size()));
        if (at.fileName().isEmpty() || !at.open(QFile::ReadOnly)) {
            return {};
        }
        s_cache[url] = QString::fromUtf8(at.readAll());
    }

    auto str = s_cache[url];
    str.replace("fill:#000000"_L1, QString(u"fill:" + color.name()));
    return QUrl(QString(u"data:image/svg+xml;utf8," + QString::fromLatin1(QUrl::toPercentEncoding(str))));
}

void KCMUser::load()
{
    Q_EMIT reset();
}

#include "kcm.moc"

#include "moc_kcm.cpp"
