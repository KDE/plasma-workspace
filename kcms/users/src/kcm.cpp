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

// Work around QTBUG-100458
inline auto asyncCall(OrgFreedesktopAccountsInterface *ptr, const QString &method, const QVariantList &arguments)
{
    auto mc = QDBusMessage::createMethodCall(ptr->service(), ptr->path(), ptr->interface(), method);
    mc.setArguments(arguments);
    mc.setInteractiveAuthorizationAllowed(true);
    return QDBusConnection::systemBus().asyncCall(mc);
}

KCMUser::KCMUser(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, data, args)
    , m_dbusInterface(new OrgFreedesktopAccountsInterface(QStringLiteral("org.freedesktop.Accounts"),
                                                          QStringLiteral("/org/freedesktop/Accounts"),
                                                          QDBusConnection::systemBus(),
                                                          this))
    , m_model(new UserModel(this))
    , m_fingerprintModel(new FingerprintModel(this))
{
    qmlRegisterUncreatableType<User>("org.kde.plasma.kcm.users", 1, 0, "User", QString());
    qmlRegisterUncreatableType<FprintDevice>("FingerprintModel", 1, 0, "FprintDevice", QString());
    qmlRegisterType<Finger>("FingerprintModel", 1, 0, "Finger");
    qmlRegisterType<MaskMouseArea>("org.kde.plasma.kcm.users", 1, 0, "MaskMouseArea");
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
    QDBusPendingReply<QDBusObjectPath> reply = asyncCall(m_dbusInterface, "CreateUser", {name, realName, static_cast<qint32>(isAdmin)});
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
    QDBusPendingReply<> reply = asyncCall(m_dbusInterface, "DeleteUser", {id, deleteHome});
    reply.waitForFinished();
    if (reply.isError()) {
        return false;
    } else {
        return true;
    }
}

KCMUser::~KCMUser()
{
}

void KCMUser::save()
{
    KQuickAddons::ConfigModule::save();
    Q_EMIT apply();
}

// Grab the initials of a string
QString KCMUser::initializeString(const QString &stringToGrabInitialsOf)
{
    if (stringToGrabInitialsOf.isEmpty())
        return "";

    auto normalized = stringToGrabInitialsOf.normalized(QString::NormalizationForm_D);
    if (normalized.contains(" ")) {
        QStringList split = normalized.split(" ");
        auto first = split.first();
        auto last = split.last();
        if (first.isEmpty()) {
            return QString(last.front());
        }
        if (last.isEmpty()) {
            return QString(first.front());
        }
        return QString(first.front()) + last.front();
    } else {
        return QString(normalized.front());
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
        QFile at(url.toLocalFile());
        if (!at.open(QFile::ReadOnly)) {
            return QUrl();
        }
        s_cache[url] = QString::fromUtf8(at.readAll());
    }

    auto str = s_cache[url];
    str.replace("fill:#000000", "fill:" + color.name());
    return QUrl("data:image/svg+xml;utf8," + QUrl::toPercentEncoding(str));
}

void KCMUser::load()
{
    Q_EMIT reset();
}

#include "kcm.moc"
