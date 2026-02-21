/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "slidefiltermodel.h"
#include "model/abstractimagelistmodel.h"
#include "slidemodel.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QRandomGenerator>

#include <KIO/OpenFileManagerWindowJob>

#include <algorithm>

namespace
{
inline QString getDisplayName(const QModelIndex &modelIndex)
{
    return modelIndex.data(Qt::DisplayRole).toString();
}

inline QString getLocalFilePath(const QModelIndex &modelIndex)
{
    return modelIndex.data(ImageRoles::SourceRole).toUrl().toLocalFile();
}

inline QString getFilePathWithDir(const QFileInfo &fileInfo)
{
    return fileInfo.canonicalPath().append(QDir::separator());
}
}

SlideFilterModel::SlideFilterModel(const QBindable<QSize> &targetSize,
                                   const QBindable<bool> &usedInConfig,
                                   const QBindable<SortingMode::Mode> &sortingMode,
                                   const QBindable<bool> &slideshowFoldersFirst,
                                   QObject *parent)
    : QSortFilterProxyModel{parent}
    , m_SortingMode(sortingMode.makeBinding())
    , m_SortingFoldersFirst(slideshowFoldersFirst.makeBinding())
    , m_usedInConfig(usedInConfig.makeBinding())
    , m_targetSize(targetSize.makeBinding())
    , m_random(m_randomDevice())
{
    srand(time(nullptr));
    setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    m_usedInConfigNotifier = m_usedInConfig.addNotifier([this] {
        invalidateRowsFilter();
    });

    auto sortCallback = [this] {
        if (m_SortingMode == SortingMode::Random && !m_usedInConfig) {
            buildRandomOrder();
        }
        QSortFilterProxyModel::invalidate();
        sort(0);
    };
    m_SortingModeNotifier = m_SortingMode.addNotifier(sortCallback);
    m_slideshowFoldersFirstNotifier = m_SortingFoldersFirst.addNotifier(sortCallback);
}

QHash<int, QByteArray> SlideFilterModel::roleNames() const
{
    if (sourceModel()) {
        return sourceModel()->roleNames();
    }

    return QSortFilterProxyModel::roleNames();
}

bool SlideFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    auto index = sourceModel()->index(source_row, 0, source_parent);
    return m_usedInConfig || index.data(ImageRoles::ToggleRole).toBool();
}

void SlideFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (this->sourceModel()) {
        disconnect(this->sourceModel(), nullptr, this, nullptr);
        // If the new model and the old model is the same model, QSortFilterProxyModel will not invalidate the filter
        QSortFilterProxyModel::setSourceModel(nullptr);
    }
    if (sourceModel) {
        connect(sourceModel, &QAbstractItemModel::modelReset, this, &SlideFilterModel::buildRandomOrder);
        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &, int first, int last) {
            if (m_SortingMode != SortingMode::Random || m_usedInConfig) {
                return;
            }
            const int old_count = m_randomOrder.size();
            if (first < old_count /* Not appended to end */) {
                // Increase the existing row numbers that >= first to make space for new items
                for (auto &row : m_randomOrder) {
                    if (row >= first) {
                        row += last - first + 1;
                    }
                }
            }
            // Then append new row numbers
            m_randomOrder.resize(this->sourceModel()->rowCount());
            std::iota(std::next(m_randomOrder.begin(), old_count), m_randomOrder.end(), first); // first to last
            std::shuffle(std::next(m_randomOrder.begin(), old_count), m_randomOrder.end(), m_random);
        });
        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &, int first, int last) {
            if (m_SortingMode != SortingMode::Random || m_usedInConfig) {
                return;
            }

            const int old_count = m_randomOrder.size();
            m_randomOrder.erase(std::remove_if(m_randomOrder.begin(),
                                               m_randomOrder.end(),
                                               [first, last](int v) {
                                                   return v >= first && v <= last;
                                               }),
                                m_randomOrder.end());

            if (last + 1 < old_count /* Not the last one */) {
                // Decrease the remaining row numbers that > last
                for (auto &row : m_randomOrder) {
                    if (row > last) {
                        row -= last - first + 1;
                    }
                }
            }
        });
    }
    // Update random order before QSortFilterProxyModel sorts
    QSortFilterProxyModel::setSourceModel(sourceModel);
    if (m_SortingMode == SortingMode::Random && !m_usedInConfig) {
        buildRandomOrder();
    }
}

bool SlideFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    constexpr Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    switch (m_SortingMode) {
    case SortingMode::Random:
        if (m_usedInConfig) {
            return source_left.row() < source_right.row();
        }
        return m_randomOrder.indexOf(source_left.row()) < m_randomOrder.indexOf(source_right.row());
    case SortingMode::Alphabetical:
        if (m_SortingFoldersFirst) {
            QFileInfo leftFile(getLocalFilePath(source_left));
            QFileInfo rightFile(getLocalFilePath(source_right));
            QString leftFilePath = getFilePathWithDir(leftFile);
            QString rightFilePath = getFilePathWithDir(rightFile);
            const QString leftImage(getDisplayName(source_left));
            const QString rightImage(getDisplayName(source_right));

            if (leftFilePath == rightFilePath) {
                return QString::localeAwareCompare(leftImage, rightImage) < 0;
            } else if (leftFilePath.startsWith(rightFilePath, cs)) {
                return true;
            } else if (rightFilePath.startsWith(leftFilePath, cs)) {
                return false;
            } else {
                return QString::localeAwareCompare(leftFilePath, rightFilePath) < 0;
            }
        } else {
            const QString leftImage(getDisplayName(source_left));
            const QString rightImage(getDisplayName(source_right));
            return QString::localeAwareCompare(leftImage, rightImage) < 0;
        }
    case SortingMode::AlphabeticalReversed:
        if (m_SortingFoldersFirst) {
            QFileInfo leftFile(getLocalFilePath(source_left));
            QFileInfo rightFile(getLocalFilePath(source_right));
            QString leftFilePath = getFilePathWithDir(leftFile);
            QString rightFilePath = getFilePathWithDir(rightFile);
            const QString leftImage(getDisplayName(source_left));
            const QString rightImage(getDisplayName(source_right));

            if (leftFilePath == rightFilePath) {
                return QString::localeAwareCompare(leftImage, rightImage) > 0;
            } else if (leftFilePath.startsWith(rightFilePath, cs)) {
                return true;
            } else if (rightFilePath.startsWith(leftFilePath, cs)) {
                return false;
            } else {
                return QString::localeAwareCompare(leftFilePath, rightFilePath) > 0;
            }
        } else {
            const QString leftImage(getDisplayName(source_left));
            const QString rightImage(getDisplayName(source_right));
            return QString::localeAwareCompare(leftImage, rightImage) > 0;
        }
    case SortingMode::Modified: // oldest first
    {
        QFileInfo leftFile(getLocalFilePath(source_left));
        QFileInfo rightFile(getLocalFilePath(source_right));
        return leftFile.lastModified() < rightFile.lastModified();
    }
    case SortingMode::ModifiedReversed: // newest first
    {
        QFileInfo leftFile(getLocalFilePath(source_left));
        QFileInfo rightFile(getLocalFilePath(source_right));
        return leftFile.lastModified() > rightFile.lastModified();
    }
    }
    Q_UNREACHABLE();
}

void SlideFilterModel::invalidate()
{
    if (m_SortingMode == SortingMode::Random && !m_usedInConfig) {
        std::shuffle(m_randomOrder.begin(), m_randomOrder.end(), m_random);
    }
    QSortFilterProxyModel::invalidate();
    sort(0);
}

void SlideFilterModel::invalidateFilter()
{
    QSortFilterProxyModel::invalidateFilter();
}

void SlideFilterModel::swapFirstWithRandom()
{
    const int size = rowCount();
    if (size < 2)
        return;

    // [start, end)
    const int pos = QRandomGenerator::global()->bounded(1, size);
    // get the randomOrder index of the sourceModel index of the proxy index
    const QModelIndex srcIdxA = mapToSource(index(0, 0));
    const QModelIndex srcIdxB = mapToSource(index(pos, 0));
    const int rndIdxA = m_randomOrder.indexOf(srcIdxA.row());
    const int rndIdxB = m_randomOrder.indexOf(srcIdxB.row());
    // and finally swap
    m_randomOrder.swapItemsAt(rndIdxA, rndIdxB);

    QSortFilterProxyModel::invalidate();
    sort(0);
}

int SlideFilterModel::indexOf(const QString &path)
{
    if (!sourceModel())
        return -1;

    auto sourceIndex = sourceModel()->index(static_cast<SlideModel *>(sourceModel())->indexOf(path), 0);
    return mapFromSource(sourceIndex).row();
}

void SlideFilterModel::openContainingFolder(int rowIndex)
{
    auto slideModel = static_cast<SlideModel *>(sourceModel());
    slideModel->openContainingFolder(mapToSource(index(rowIndex, 0)).row());
}

void SlideFilterModel::selectAllSlides()
{
    for (int rowIndex = 0; rowIndex < sourceModel()->rowCount(); rowIndex++) {
        setData(index(rowIndex, 0), true, ImageRoles::ToggleRole);
    }
}

void SlideFilterModel::deselectAllSlides()
{
    for (int rowIndex = 0; rowIndex < sourceModel()->rowCount(); rowIndex++) {
        setData(index(rowIndex, 0), false, ImageRoles::ToggleRole);
    }
}

void SlideFilterModel::buildRandomOrder()
{
    if (sourceModel()) {
        m_randomOrder.resize(sourceModel()->rowCount());
        std::iota(m_randomOrder.begin(), m_randomOrder.end(), 0);
        std::shuffle(m_randomOrder.begin(), m_randomOrder.end(), m_random);
    }
}
