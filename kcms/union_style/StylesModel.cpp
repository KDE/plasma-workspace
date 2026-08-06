// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

#include "StylesModel.h"

#include <QCollator>
#include <QStandardPaths>

#include <Union/StyleRegistry.h>

using namespace Qt::StringLiterals;

StylesModel::StylesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    refresh();
}

QHash<int, QByteArray> StylesModel::roleNames() const
{
    return {
        {StyleIdRole, "styleId"_ba},
        {NameRole, "name"_ba},
        {DescriptionRole, "description"_ba},
        {CanUninstallRole, "canUninstall"_ba},
    };
}

int StylesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_styles.size();
}

QVariant StylesModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return QVariant{};
    }

    auto &style = m_styles.at(index.row());

    switch (role) {
    case StyleIdRole:
        return style.id();
    case NameRole:
        return style.name();
    case DescriptionRole:
        return style.description();
    case CanUninstallRole: {
        const auto writeablePath =
            std::filesystem::path(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation).toStdString()) / "union" / "styles";
        return style.path().parent_path() == writeablePath;
    }
    default:
        return QVariant{};
    }
}

int StylesModel::indexOf(const QString &styleId)
{
    for (int i = 0; i < m_styles.size(); ++i) {
        if (m_styles.at(i).id() == styleId) {
            return i;
        }
    }
    return -1;
}

void StylesModel::refresh()
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
