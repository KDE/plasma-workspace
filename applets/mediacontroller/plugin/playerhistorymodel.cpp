/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "playerhistorymodel.h"

#include <QCollator>
#include <QCoreApplication>
#include <QFuture>
#include <QFutureWatcher>
#include <QPointer>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KLocalizedString>

using namespace Qt::StringLiterals;

PlayerHistoryModel::PlayerHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_config(
          KSharedConfig::openConfig(QCoreApplication::applicationName() + "/playerhistoryrc"_L1, KConfig::SimpleConfig, QStandardPaths::GenericCacheLocation))
{
    auto watcher = new QFutureWatcher<QList<PlayerInformation>>();
    connect(watcher, &QFutureWatcher<QList<PlayerInformation>>::finished, this, [this, watcher] {
        beginResetModel();
        m_data = watcher->future().result();
        m_data.emplace_back(i18nc("@title", "Other…"), u"application-x-shellscript"_s, QString());
        m_ready = true; // Before endResetModel so indexOf can work
        endResetModel();
    });
    connect(watcher, &QFutureWatcher<QList<PlayerInformation>>::finished, watcher, &QObject::deleteLater);
    watcher->setFuture(QtConcurrent::run(&PlayerHistoryModel::readPlayers, this));
}

PlayerHistoryModel::~PlayerHistoryModel()
{
}

QHash<int, QByteArray> PlayerHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
    roles.emplace(IdentityRole, QByteArrayLiteral("identity"));
    return roles;
}

int PlayerHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

QVariant PlayerHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }

    const PlayerInformation &info = m_data[index.row()];
    Q_ASSERT(!info.identity.isEmpty());

    switch (role) {
    case Qt::DisplayRole:
        return info.displayName.isEmpty() ? info.identity : info.displayName;

    case Qt::DecorationRole:
        return info.icon.isEmpty() ? u"emblem-music-symbolic"_s : info.icon;

    case IdentityRole:
        return info.identity;

    default:
        return {};
    }
}

int PlayerHistoryModel::indexOf(const QString &identity) const
{
    if (!m_ready || identity.isEmpty()) {
        return -1;
    }
    auto it = std::find(m_data.cbegin(), m_data.cend(), identity);
    if (it == m_data.cend()) {
        return -1;
    }
    return std::distance(m_data.cbegin(), it);
}

void PlayerHistoryModel::rememberPlayer(const QString &identity)
{
    if (!m_ready || identity.isEmpty() || m_config->group(u"Players"_s).hasGroup(identity)) {
        return;
    }

    PlayerInformation info = readPlayer(identity);

    beginInsertRows(QModelIndex(), m_data.size() - 1, m_data.size() - 1);
    m_data.emplace(m_data.size() - 1, std::move(info));
    endInsertRows();

    m_config->group(u"Players"_s).group(identity).writeEntry(u"saved"_s, 1);
    m_config->sync();
}

void PlayerHistoryModel::forgetAllPlayers()
{
    if (m_data.size() < 2) {
        Q_ASSERT_X(false, Q_FUNC_INFO, qPrintable(QString::number(m_data.size())));
        return;
    }

    beginRemoveRows(QModelIndex(), 0, m_data.size() - 2);
    for (int i = 0; i < m_data.size() - 1; ++i) {
        m_config->group(u"Players"_s).deleteGroup(m_data[i].identity);
    }
    m_data = {std::move(*m_data.rbegin())};
    endRemoveRows();

    m_config->sync();
}

PlayerHistoryModel::PlayerInformation PlayerHistoryModel::readPlayer(const QString &identity) const
{
    const KDesktopFile desktop(identity + QLatin1String(".desktop"));
    PlayerInformation info;
    info.displayName = desktop.readName();
    info.identity = identity;
    info.icon = desktop.readIcon();
    return info;
}

QList<PlayerHistoryModel::PlayerInformation> PlayerHistoryModel::readPlayers()
{
    const QStringList players = m_config->group(u"Players"_s).groupList();
    decltype(m_data) data;
    for (const QString &player : players) {
        data.emplace_back(readPlayer(player));
    }
    QCollator c;
    std::sort(data.begin(), data.end(), [&c](const PlayerInformation &left, const PlayerInformation &right) {
        return c.compare(left.identity, right.identity) < 0;
    });
    return data;
}