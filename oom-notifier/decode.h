// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#pragma once

#include <QString>

inline std::optional<char> narrow(int i)
{
    auto c = static_cast<char>(i);
    if (c != i) {
        return {};
    }
    return c;
}

inline QString decodeUnitName(const QStringView &input)
{
    QByteArray decoded;
    decoded.reserve(input.size());
    const QByteArray bytes = input.toUtf8();
    for (auto it = bytes.cbegin(); it != bytes.cend(); it++) {
        const auto &c = *it;
        if (c != QLatin1Char('_')) { // literal character
            decoded.append(c);
            continue;
        }

        const auto nextIt = it + 2;
        if (nextIt == bytes.cend()) { // literal _ because there aren't enough characters to decode
            decoded.append(c);
            continue;
        }

        QByteArray hex;
        hex.append(it + 1, 2);
        bool ok = false;
        constexpr auto hexBase = 16;
        const auto decodedInt = hex.toInt(&ok, hexBase);
        const auto decodedChar = narrow(decodedInt);
        if (!ok || !decodedChar.has_value()) { // not a valid hex -> consider this a literal _
            decoded.append(c);
            continue;
        }

        decoded.append(decodedChar.value());
        it = nextIt;
    }
    return QString::fromUtf8(decoded);
}

// https://systemd.io/DESKTOP_ENVIRONMENTS/
inline QStringView unitNameToServiceName(QStringView unitName)
{
    Q_ASSERT_X(!unitName.contains(QLatin1Char('/')), Q_FUNC_INFO, unitName.toUtf8().constData());

    if (!unitName.startsWith(QLatin1String("app-"))) {
        return {};
    }

    constexpr auto separator = QLatin1Char('-');

    // Always chop the app- prefix off
    auto name = unitName.mid(unitName.indexOf(separator) + 1);
    // Sometimes chop the @uuid.scope suffix off
    if (auto index = name.lastIndexOf(QLatin1Char('@')); index >= 0) {
        name.truncate(index);
    } else if (auto index = name.lastIndexOf(separator); index >= 0) {
        // Sometimes can be -uuid.scope instead
        name.truncate(index);
    }
    // At this point the only remaining part could be a launcher prefix, technically desktop names may contain hyphens
    // too so we can but hope that it's not a leading part. Otherwise resolution fails, sucks but not the end of the world.
    if (auto separatorIndex = name.indexOf(separator); separatorIndex > 0 && separatorIndex < name.indexOf(QLatin1Char('.'))) {
        name = name.mid(separatorIndex + 1);
    }

    return name;
}
