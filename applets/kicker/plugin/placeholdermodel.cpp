/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2017 Ivan Cukic <ivan.cukic@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "placeholdermodel.h"
#include "actionlist.h"
#include "debug.h"
#include <chrono>

using namespace std::chrono_literals;

PlaceholderModel::PlaceholderModel(QObject *parent)
    : AbstractModel(parent)
    , m_dropPlaceholderIndex(-1)
    , m_isTriggerInhibited(false)
{
    connect(&m_triggerInhibitor, &QTimer::timeout, this, [&] {
        qCDebug(KICKER_DEBUG) << "%%% Inhibit stopped";
        m_isTriggerInhibited = false;
    });

    m_triggerInhibitor.setInterval(500ms);
    m_triggerInhibitor.setSingleShot(true);
}

void PlaceholderModel::inhibitTriggering()
{
    qCDebug(KICKER_DEBUG) << "%%% Inhibit started";
    m_isTriggerInhibited = true;
    m_triggerInhibitor.start();
}

PlaceholderModel::~PlaceholderModel()
{
}

QString PlaceholderModel::description() const
{
    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->description();

    } else {
        return QString();
    }
}

QAbstractItemModel *PlaceholderModel::sourceModel() const
{
    return m_sourceModel;
}

void PlaceholderModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    disconnectSignals();

    beginResetModel();

    m_sourceModel = sourceModel;

    connectSignals();

    endResetModel();

    Q_EMIT countChanged();
    Q_EMIT sourceModelChanged();
    Q_EMIT descriptionChanged();
}

bool PlaceholderModel::canFetchMore(const QModelIndex &parent) const
{
    return m_sourceModel && m_sourceModel->canFetchMore(indexToSourceIndex(parent));
}

void PlaceholderModel::fetchMore(const QModelIndex &parent)
{
    if (m_sourceModel) {
        m_sourceModel->fetchMore(indexToSourceIndex(parent));
    }
}

QModelIndex PlaceholderModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return m_sourceModel ? createIndex(row, column) : QModelIndex();
}

QModelIndex PlaceholderModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)

    return QModelIndex();
}

QVariant PlaceholderModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (m_dropPlaceholderIndex == row) {
        switch (role) {
        case Kicker::IsDropPlaceholderRole:
            return true;

            // TODO: Maybe it would be nice to show something here?
            // case Qt::DisplayRole:
            //     return "placeholder";
            //
            // case Qt::DecorationRole:
            //     return "select";

        default:
            return QVariant();
        }
    }

    return m_sourceModel ? m_sourceModel->data(indexToSourceIndex(index), role) : QVariant();
}

int PlaceholderModel::rowCount(const QModelIndex &parent) const
{
    if (!m_sourceModel || parent.isValid()) {
        return 0;
    }

    return m_sourceModel->rowCount() + (m_dropPlaceholderIndex != -1 ? 1 : 0);
}

QModelIndex PlaceholderModel::indexToSourceIndex(const QModelIndex &index) const
{
    if (!m_sourceModel || !index.isValid()) {
        return QModelIndex();
    }

    const auto row = index.row();
    const auto column = index.column();

    return index.parent().isValid() ?
                                    // We do not support tree models
        QModelIndex()
                                    :

                                    // If we are on top-level, lets add a placeholder
        m_sourceModel->index(row - (m_dropPlaceholderIndex != -1 && row > m_dropPlaceholderIndex ? 1 : 0), column, QModelIndex());
}

int PlaceholderModel::sourceRowToRow(int sourceRow) const
{
    return sourceRow + (m_dropPlaceholderIndex != -1 && sourceRow >= m_dropPlaceholderIndex ? 1 : 0);
}

int PlaceholderModel::rowToSourceRow(int row) const
{
    return row == m_dropPlaceholderIndex ? -1 : row - (m_dropPlaceholderIndex != -1 && row > m_dropPlaceholderIndex ? 1 : 0);
}

QModelIndex PlaceholderModel::sourceIndexToIndex(const QModelIndex &sourceIndex) const
{
    if (!m_sourceModel || !sourceIndex.isValid()) {
        return QModelIndex();
    }

    const auto sourceRow = sourceIndex.row();
    const auto sourceColumn = sourceIndex.column();

    return sourceIndex.parent().isValid() ?
                                          // We do not support tree-models
        QModelIndex()
                                          :

                                          // If we are on top-level, lets add a placeholder
        index(sourceRowToRow(sourceRow), sourceColumn, QModelIndex());
}

bool PlaceholderModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    if (m_isTriggerInhibited)
        return false;

    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->trigger(rowToSourceRow(row), actionId, argument);

    } else {
        return false;
    }
}

QString PlaceholderModel::labelForRow(int row)
{
    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->labelForRow(rowToSourceRow(row));

    } else {
        return QString();
    }
}

AbstractModel *PlaceholderModel::modelForRow(int row)
{
    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->modelForRow(rowToSourceRow(row));

    } else {
        return nullptr;
    }
}

AbstractModel *PlaceholderModel::favoritesModel()
{
    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->favoritesModel();

    } else {
        return AbstractModel::favoritesModel();
    }
}

int PlaceholderModel::separatorCount() const
{
    if (auto abstractModel = qobject_cast<AbstractModel *>(m_sourceModel)) {
        return abstractModel->separatorCount();

    } else {
        return 0;
    }
}

void PlaceholderModel::reset()
{
    beginResetModel();
    endResetModel();
    Q_EMIT countChanged();
    Q_EMIT separatorCountChanged();
}

void PlaceholderModel::connectSignals()
{
    if (!m_sourceModel) {
        return;
    }

    const auto sourceModelPtr = m_sourceModel.data();

    connect(sourceModelPtr, SIGNAL(destroyed()), this, SLOT(reset()));

    connect(sourceModelPtr, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &from, const QModelIndex &to, const QVector<int> &roles) {
        Q_EMIT dataChanged(sourceIndexToIndex(from), sourceIndexToIndex(to), roles);
    });

    connect(sourceModelPtr, &QAbstractItemModel::rowsAboutToBeInserted, this, [this](const QModelIndex &parent, int from, int to) {
        if (parent.isValid()) {
            qCWarning(KICKER_DEBUG) << "We do not support tree models";

        } else {
            beginInsertRows(QModelIndex(), sourceRowToRow(from), sourceRowToRow(to));
        }
    });

    connect(sourceModelPtr, &QAbstractItemModel::rowsInserted, this, [this] {
        endInsertRows();
        Q_EMIT countChanged();
    });

    connect(sourceModelPtr,
            &QAbstractItemModel::rowsAboutToBeMoved,
            this,
            [this](const QModelIndex &source, int from, int to, const QModelIndex &dest, int destRow) {
                if (source.isValid() || dest.isValid()) {
                    qCWarning(KICKER_DEBUG) << "We do not support tree models";

                } else {
                    beginMoveRows(QModelIndex(), sourceRowToRow(from), sourceRowToRow(to), QModelIndex(), sourceRowToRow(destRow));
                }
            });

    connect(sourceModelPtr, &QAbstractItemModel::rowsMoved, this, [this] {
        endMoveRows();
    });

    connect(sourceModelPtr, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this](const QModelIndex &parent, int from, int to) {
        if (parent.isValid()) {
            qCWarning(KICKER_DEBUG) << "We do not support tree models";

        } else {
            beginRemoveRows(QModelIndex(), sourceRowToRow(from), sourceRowToRow(to));
        }
    });

    connect(sourceModelPtr, &QAbstractItemModel::rowsRemoved, this, [this] {
        endRemoveRows();
        Q_EMIT countChanged();
    });

    connect(sourceModelPtr, &QAbstractItemModel::modelAboutToBeReset, this, [this] {
        beginResetModel();
    });

    connect(sourceModelPtr, &QAbstractItemModel::modelReset, this, [this] {
        endResetModel();
        Q_EMIT countChanged();
    });

    // We do not have persistant indices
    // connect(sourceModelPtr, &QAbstractItemModel::layoutAboutToBeChanged),
    //         this, &PlaceholderModel::layoutAboutToBeChanged);
    // connect(sourceModelPtr, &QAbstractItemModel::layoutChanged),
    //         this, SIGNAL(layoutChanged(QList<QPersistentModelIndex>,QAbstractItemModel::LayoutChangeHint)),
    //         Qt::UniqueConnection);
}

void PlaceholderModel::disconnectSignals()
{
    if (!m_sourceModel) {
        return;
    }

    disconnect(m_sourceModel, nullptr, this, nullptr);
}

int PlaceholderModel::dropPlaceholderIndex() const
{
    return m_dropPlaceholderIndex;
}

void PlaceholderModel::setDropPlaceholderIndex(int index)
{
    if (index == m_dropPlaceholderIndex)
        return;

    inhibitTriggering();

    if (index == -1 && m_dropPlaceholderIndex != -1) {
        // Removing the placeholder
        beginRemoveRows(QModelIndex(), m_dropPlaceholderIndex, m_dropPlaceholderIndex);
        m_dropPlaceholderIndex = index;
        endRemoveRows();

        Q_EMIT countChanged();

    } else if (index != -1 && m_dropPlaceholderIndex == -1) {
        // Creating the placeholder
        beginInsertRows(QModelIndex(), index, index);
        m_dropPlaceholderIndex = index;
        endInsertRows();

        Q_EMIT countChanged();

    } else if (m_dropPlaceholderIndex != index) {
        // Moving the placeholder
        int modelTo = index + (index > m_dropPlaceholderIndex ? 1 : 0);

        if (beginMoveRows(QModelIndex(), m_dropPlaceholderIndex, m_dropPlaceholderIndex, QModelIndex(), modelTo)) {
            m_dropPlaceholderIndex = index;
            endMoveRows();
        }
    }

    Q_EMIT dropPlaceholderIndexChanged();
}
