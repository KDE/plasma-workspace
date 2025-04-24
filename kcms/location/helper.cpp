/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "helper.h"

#include <KAuth/HelperSupport>
#include <KConfigGroup>
#include <KConfig>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static QString staticPositionFilePath()
{
    return QStringLiteral("/etc/geolocation");
}

static QString forceStaticPositionFilePath()
{
    return QStringLiteral("/etc/geoclue/conf.d/90-static-source.conf");
}

static QString globalToggleFilePath()
{
    return QStringLiteral("/etc/geoclue/conf.d/90-global-toggle.conf");
}

static QVariantMap parseConfig(const QString &filePath)
{
    QVariantMap parsed;

    const KConfig config(filePath, KConfig::SimpleConfig);
    const QStringList groupNames = config.groupList();
    for (const QString &groupName : groupNames) {
        const KConfigGroup group = config.group(groupName);
        const QVariant enabled = group.readEntry("enable");
        if (!enabled.isNull()) {
            parsed[groupName] = enabled;
        }
    }

    return parsed;
}

static QVariantMap mergeConfigs(const QVariantMap &a, const QVariantMap &b)
{
    QVariantMap merged = a;
    for (const auto &[key, value] : b.asKeyValueRange()) {
        merged[key] = value;
    }
    return merged;
}

static QVariantMap geoClueConfig()
{
    QVariantMap config = parseConfig(QStringLiteral("/etc/geoclue/geoclue.conf"));

    const QDir confd(QStringLiteral("/etc/geoclue/conf.d"));
    QStringList configFiles = confd.entryList({QStringLiteral("*.conf")}, QDir::Files | QDir::NoDotAndDotDot, QDir::LocaleAware);
    for (const QString &configFile : configFiles) {
        const QString configFilePath = confd.absoluteFilePath(configFile);
        config = mergeConfigs(config, parseConfig(configFilePath));
    }

    return config;
}

static QStringList enabledSources(const QVariantMap &config)
{
    QStringList sources;

    for (const auto &[key, value] : config.asKeyValueRange()) {
        if (value.toBool()) {
            sources.append(key);
        }
    }

    return sources;
}

static QStringList availableSources(const QVariantMap &config)
{
    return config.keys();
}

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

static bool makeReadableByGeoclue(const QString &filePath)
{
    if (passwd *entry = getpwnam("geoclue")) {
        const QByteArray nativeFilePath = filePath.toLocal8Bit();
        if (chown(nativeFilePath.data(), entry->pw_uid, entry->pw_gid) != -1) {
            if (chmod(nativeFilePath.data(), S_IRUSR | S_IWUSR) == -1) {
                qWarning() << "Failed to set file mode of" << nativeFilePath << "to 600:" << strerror(errno);
                return false;
            }
        } else {
            qWarning() << "Failed to change ownership of" << nativeFilePath << "to geoclue user:" << strerror(errno);
            return false;
        }
    } else {
        qWarning() << "Failed to query information about geoclue user:" << strerror(errno);
        return false;
    }
    return true;
}

Helper::Helper()
{
}

KAuth::ActionReply Helper::reconfigure(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    // TODO: File an upstream issue about a better way to ask geoclue to use the new configuration.
    const QString systemctl = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    if (!systemctl.isEmpty()) {
        QProcess::execute(systemctl, {QStringLiteral("restart"), QStringLiteral("geoclue.service")});
    }
    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply Helper::query(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    const auto config = geoClueConfig();

    KAuth::ActionReply reply = KAuth::ActionReply::SuccessReply();
    reply.addData(QStringLiteral("enabledSources"), enabledSources(config));
    reply.addData(QStringLiteral("availableSources"), availableSources(config));

    if (QFile file(staticPositionFilePath()); file.open(QFile::ReadOnly | QFile::Text)) {
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

        reply.addData(QStringLiteral("staticLocation"),
                      QVariantMap{
                          {QStringLiteral("latitude"), latitude.value_or(0.0)},
                          {QStringLiteral("longitude"), longitude.value_or(0.0)},
                          {QStringLiteral("altitude"), altitude.value_or(0.0)},
                      });
    }

    return reply;
}

KAuth::ActionReply Helper::enable(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    if (QFile file(globalToggleFilePath()); !file.remove()) {
        KAuth::ActionReply reply(KAuth::ActionReply::HelperErrorType);
        reply.setErrorCode(KAuth::ActionReply::BackendError);
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply Helper::disable(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    // See https://gitlab.freedesktop.org/geoclue/geoclue/-/issues/211 for a more future-proof way
    // to toggle the global geolocation status.
    const QStringList availableSources = ::availableSources(geoClueConfig());
    KConfig config(globalToggleFilePath(), KConfig::SimpleConfig);
    for (const QString &source : availableSources) {
        config.group(source).writeEntry(QStringLiteral("enable"), false);
    }

    if (!config.sync()) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Failed to disable all location sources"));
        return reply;
    }

    if (!makeReadableByGeoclue(globalToggleFilePath())) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Failed to disable all location sources"));
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply Helper::setstaticlocation(const QVariantMap &parameters)
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

    // Disable all location sources other than the static-source. See https://gitlab.freedesktop.org/geoclue/geoclue/-/issues/212
    // for a more future-proof option.
    {
        const QStringList availableSources = ::availableSources(geoClueConfig());
        KConfig staticLocationConfig(forceStaticPositionFilePath(), KConfig::SimpleConfig);
        for (const QString &source : availableSources) {
            if (source != QLatin1String("static-source")) {
                staticLocationConfig.group(source).writeEntry(QStringLiteral("enable"), false);
            }
        }

        if (!staticLocationConfig.sync()) {
            auto reply = KAuth::ActionReply::HelperErrorReply();
            reply.setErrorDescription(QStringLiteral("Failed to force the static-source source"));
            return reply;
        }

        if (!makeReadableByGeoclue(forceStaticPositionFilePath())) {
            auto reply = KAuth::ActionReply::HelperErrorReply();
            reply.setErrorDescription(QStringLiteral("Failed to force the static-source source"));
            return reply;
        }
    }

    // Write the static position to /etc/geolocation and permit only the geoclue daemon to read (as suggested by the docs).
    if (QFile file(staticPositionFilePath()); !file.open(QFile::WriteOnly | QFile::Text)) {
        auto reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(QStringLiteral("Failed to write static position: ") + file.errorString());
        return reply;
    } else {
        const qreal accuracy = 1.0;

        QTextStream stream(&file);
        stream << latitude.value() << '\n';
        stream << longitude.value() << '\n';
        stream << altitude.value() << '\n';
        stream << accuracy << '\n';
    }

    makeReadableByGeoclue(staticPositionFilePath());
    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply Helper::unsetstaticlocation(const QVariantMap &parameters)
{
    Q_UNUSED(parameters)

    if (QFile file(staticPositionFilePath()); !file.remove()) {
        KAuth::ActionReply reply(KAuth::ActionReply::HelperErrorType);
        reply.setErrorCode(KAuth::ActionReply::BackendError);
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    if (QFile file(forceStaticPositionFilePath()); !file.remove()) {
        KAuth::ActionReply reply(KAuth::ActionReply::HelperErrorType);
        reply.setErrorCode(KAuth::ActionReply::BackendError);
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("org.kde.kcm.location", Helper)
