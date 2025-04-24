/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "helper.h"

#include <KAuth/HelperSupport>

#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define GEOLOCATION_SENTINEL_FILE "/etc/geolocation"

static QStringView stripComment(const QStringView &line)
{
    const int index = line.indexOf(QLatin1Char('#'));
    if (index == -1) {
        return line;
    }

    return line.first(index).trimmed();
}

static std::optional<qreal> parseDouble(const QVariant &value)
{
    bool ok = false;
    const qreal ret = value.toDouble(&ok);
    if (ok) {
        return ret;
    }

    return std::nullopt;
}

static void reconfigureGeoclue()
{
    // TODO: File an upstream issue about a better way to ask geoclue to use the new configuration.
    const QString systemctl = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    if (!systemctl.isEmpty()) {
        QProcess::execute(systemctl, {QStringLiteral("restart"), QStringLiteral("geoclue.service")});
    }
}

Helper::Helper()
{
}

KAuth::ActionReply Helper::getlocation(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)
    KAuth::ActionReply reply = KAuth::ActionReply::SuccessReply();

    QFile file(QStringLiteral(GEOLOCATION_SENTINEL_FILE));
    reply.addData(QStringLiteral("present"), file.exists());

    if (file.open(QFile::ReadOnly | QFile::Text)) {
        std::optional<qreal> latitude;
        std::optional<qreal> longitude;
        std::optional<qreal> altitude;
        std::optional<qreal> accuracy;

        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            const QStringView data = stripComment(QStringView(line).trimmed());
            if (data.isEmpty()) {
                continue;
            }

            bool ok = false;
            const qreal value = data.toDouble(&ok);
            if (!ok) {
                break;
            }

            if (!latitude) {
                latitude = value;
            } else if (!longitude) {
                longitude = value;
            } else if (!altitude) {
                altitude = value;
            } else if (!accuracy) {
                accuracy = value;
            } else {
                break;
            }
        }

        reply.addData(QStringLiteral("latitude"), latitude.value_or(0.0));
        reply.addData(QStringLiteral("longitude"), longitude.value_or(0.0));
        reply.addData(QStringLiteral("altitude"), altitude.value_or(0.0));
    }

    return reply;
}

KAuth::ActionReply Helper::setlocation(const QVariantMap &parameters)
{
    const auto latitude = parseDouble(parameters[QStringLiteral("latitude")]);
    if (!latitude) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Invalid latitude"));
        return reply;
    }

    const auto longitude = parseDouble(parameters[QStringLiteral("longitude")]);
    if (!longitude) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Invalid longitude"));
        return reply;
    }

    const auto altitude = parseDouble(parameters[QStringLiteral("altitude")]);
    if (!altitude) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Invalid altitude"));
        return reply;
    }

    if (QFile file(QStringLiteral(GEOLOCATION_SENTINEL_FILE)); !file.open(QFile::WriteOnly | QFile::Text)) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Failed to open " GEOLOCATION_SENTINEL_FILE ": ") + file.errorString());
        return reply;
    } else {
        const qreal accuracy = 1.0;

        QTextStream stream(&file);
        stream << latitude.value() << '\n';
        stream << longitude.value() << '\n';
        stream << altitude.value() << '\n';
        stream << accuracy << '\n';
    }

    if (passwd *entry = getpwnam("geoclue")) {
        if (chown(GEOLOCATION_SENTINEL_FILE, entry->pw_uid, entry->pw_gid) != -1) {
            if (chmod(GEOLOCATION_SENTINEL_FILE, S_IRUSR | S_IWUSR) == -1) {
                qWarning() << "Failed to set file mode of " GEOLOCATION_SENTINEL_FILE " to 600:" << strerror(errno);
            }
        } else {
            qWarning() << "Failed to change ownership of " GEOLOCATION_SENTINEL_FILE " to geoclue user:" << strerror(errno);
        }
    } else {
        qWarning() << "Failed to query information about geoclue user:" << strerror(errno);
    }

    reconfigureGeoclue();

    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply Helper::unsetlocation(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    QFile file(QStringLiteral(GEOLOCATION_SENTINEL_FILE));
    if (file.remove()) {
        reconfigureGeoclue();
        return KAuth::ActionReply::SuccessReply();
    }

    KAuth::ActionReply reply(KAuth::ActionReply::HelperErrorType);
    reply.setErrorCode(KAuth::ActionReply::BackendError);
    reply.setErrorDescription(file.errorString());
    return reply;
}

KAUTH_HELPER_MAIN("org.kde.kcm.location", Helper)
