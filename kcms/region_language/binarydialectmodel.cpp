/*
    binarydialectmodel.cpp
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "binarydialectmodel.h"

#include <KLocalizedString>
#include <kformat.h>

BinaryDialectModel::BinaryDialectModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BinaryDialectModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return roleNames().size();
}

QVariant BinaryDialectModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    KFormat::BinaryUnitDialect dialect = KFormat::BinaryUnitDialect::DefaultBinaryDialect;
    int kdefbase = 1024;
    switch (row) {
    case KFormat::BinaryUnitDialect::IECBinaryDialect: {
        dialect = KFormat::BinaryUnitDialect::IECBinaryDialect;
        break;
    }
    case KFormat::BinaryUnitDialect::JEDECBinaryDialect: {
        dialect = KFormat::BinaryUnitDialect::JEDECBinaryDialect;
        break;
    }
    case KFormat::BinaryUnitDialect::MetricBinaryDialect: {
        dialect = KFormat::BinaryUnitDialect::MetricBinaryDialect;
        kdefbase = 1000;
        break;
    }
    }

    switch (role) {
    case DisplayName:
        switch (dialect) {
        case KFormat::BinaryUnitDialect::IECBinaryDialect: {
            return i18nc("Binary dialect IEC, with sigle in parentheses", "International Electrotechnical Commission (IEC)");
            break;
        }
        case KFormat::BinaryUnitDialect::JEDECBinaryDialect: {
            return i18nc("Binary dialect JEDEC, with sigle in parentheses", "Joint Electron Device Engineering Council (JEDEC)");
            break;
        }
        case KFormat::BinaryUnitDialect::MetricBinaryDialect: {
            return i18nc("Binary dialect Metric, with origin in parentheses", "Metric system (SI)");
            break;
        }
        default:
            break;
        }
        break;
    case Example: {
        const KFormat f;
        return f.formatByteSize(kdefbase, 1, dialect, KFormat::BinarySizeUnits::UnitKiloByte) + " = "
            + f.formatByteSize(kdefbase, 1, dialect, KFormat::BinarySizeUnits::UnitByte);
    }
    case Description: {
        switch (dialect) {
        case KFormat::BinaryUnitDialect::IECBinaryDialect:
            return i18n("Binary -  Kibibytes (KiB), Mebibytes (MiB), Gibibytes (GiB)");
        case KFormat::BinaryUnitDialect::JEDECBinaryDialect:
            return i18n("Binary - Kilobytes (kB), Megabytes (MB), Gigabytes (GB)");
        case KFormat::BinaryUnitDialect::MetricBinaryDialect:
            return i18n("Decimal - Kilobytes (kB), Megabytes (MB), Gigabytes (GB)");
        default:
            break;
        }
    }

    default:
        Q_ASSERT(false);
        break;
    }

    return QVariant{};
}

QHash<int, QByteArray> BinaryDialectModel::roleNames() const
{
    const static QHash<int, QByteArray> roles = {{DisplayName, QByteArrayLiteral("name")},
                                                 {Example, QByteArrayLiteral("example")},
                                                 {Description, QByteArrayLiteral("description")}};
    return roles;
}
