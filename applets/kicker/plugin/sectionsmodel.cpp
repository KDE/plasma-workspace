/* SPDX-FileCopyrightText: 2023 Noah Davis <noahadvs@gmail.com>
 * SPDX-FileCopyrightText: 2023 Tanbir Jishan <tantalising007@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "sectionsmodel.h"

SectionsModel::SectionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_roleNames[Qt::DisplayRole] = QByteArrayLiteral("section");
    m_roleNames[FirstIndexRole] = QByteArrayLiteral("firstIndex");
}

QHash<int, QByteArray> SectionsModel::roleNames() const
{
    return m_roleNames;
}

QVariant SectionsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    QVariant ret;
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return ret;
    }
    if (role == Qt::DisplayRole) {
        ret = m_data.at(row).section;
    } else if (role == FirstIndexRole) {
        ret = m_data.at(row).firstIndex;
    }
    return ret;
}

int SectionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_data.size();
}

void SectionsModel::clear()
{
    m_data.clear();
}

void SectionsModel::append(const QString &section, int firstIndex)
{
    m_data.append({section, firstIndex});
}

QString SectionsModel::lastSection() const
{
    Q_ASSERT(!m_data.empty());
    return m_data.constLast().section;
}
