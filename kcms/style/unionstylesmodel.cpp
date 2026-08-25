// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

#include "unionstylesmodel.h"

#include <QCollator>
#include <QStandardPaths>

#include <Union/StyleRegistry.h>

#include "stylesmodel.h"

using namespace Qt::StringLiterals;

UnionStylesModel::UnionStylesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

QHash<int, QByteArray> UnionStylesModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"_ba},
        {StylesModel::StyleIdRole, "styleId"_ba},
        {StylesModel::StyleNameRole, "styleName"_ba},
        {StylesModel::DescriptionRole, "description"_ba},
        {StylesModel::ConfigurableRole, "configurable"_ba},
        {StylesModel::IsUnionStyleRole, "isUnionStyle"_ba},
        {StylesModel::CanUninstallRole, "canUninstall"_ba},
    };
}

int UnionStylesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_styles.size();
}

QVariant UnionStylesModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return QVariant{};
    }

    auto &style = m_styles.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case StylesModel::StyleNameRole:
        return style.name();
    case StylesModel::StyleIdRole:
        return style.id();
    case StylesModel::DescriptionRole:
        return style.description();
    case StylesModel::ConfigurableRole:
        return false;
    case StylesModel::IsUnionStyleRole:
        return true;
    case StylesModel::CanUninstallRole: {
        const auto writeablePath =
            std::filesystem::path(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).toStdString()) / "union" / "styles";
        return style.path().parent_path() == writeablePath;
    }
    default:
        return QVariant{};
    }
}

int UnionStylesModel::indexOf(const QString &styleId)
{
    for (int i = 0; i < m_styles.size(); ++i) {
        if (m_styles.at(i).id() == styleId) {
            return i;
        }
    }
    return -1;
}

void UnionStylesModel::refresh()
{
    beginResetModel();

    m_styles = Union::StyleRegistry::instance()->packageHandler()->allPackages();

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::ranges::sort(m_styles, [&collator](auto &first, auto &second) {
        return collator.compare(first.name(), second.name()) < 0;
    });

    endResetModel();
}
