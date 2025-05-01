/*
    SPDX-FileCopyrightText: 2019 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "slidemodel.h"

#include <QDir>

#include "model/imageproxymodel.h"

SlideModel::SlideModel(const QBindable<QSize> &bindableTargetSize, const QBindable<bool> &bindableUsedInConfig, QObject *parent)
    : QConcatenateTablesProxyModel(parent)
    , m_targetSize(bindableTargetSize.makeBinding())
    , m_usedInConfig(bindableUsedInConfig.makeBinding())
{
}

QHash<int, QByteArray> SlideModel::roleNames() const
{
    const auto models = sourceModels();

    if (!models.empty()) {
        return models.at(0)->roleNames();
    }

    return QConcatenateTablesProxyModel::roleNames();
}

QVariant SlideModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role == ToggleRole) {
        return m_checkedTable.value(index.data(PackageNameRole).toString(), true);
    }

    return QConcatenateTablesProxyModel::data(index, role);
}

bool SlideModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }

    if (role == ToggleRole) {
        m_checkedTable[index.data(PackageNameRole).toString()] = value.toBool();

        Q_EMIT dataChanged(index, index, {ToggleRole});
        return true;
    }

    return QConcatenateTablesProxyModel::setData(index, value, role);
}

int SlideModel::indexOf(const QString &packagePath) const
{
    int idx = -1;

    for (const auto models{sourceModels()}; auto m : models) {
        idx = static_cast<ImageProxyModel *>(m)->indexOf(packagePath);

        if (idx >= 0) {
            return mapFromSource(m->index(idx, 0)).row();
        }
    }

    return idx;
}

QStringList SlideModel::addDirs(const QStringList &dirs)
{
    QStringList added;

    for (const QString &_d : dirs) {
        if (!QFileInfo(_d).isDir()) {
            continue;
        }

        const QString d = _d.endsWith(QDir::separator()) ? _d : _d + QDir::separator();

        if (!m_models.contains(d)) {
            auto m = new ImageProxyModel({d}, QBindable<QSize>(&m_targetSize), QBindable<bool>(&m_usedInConfig), this);
            m_models.insert(d, m);
            added.append(d);

            if (m->loading().value()) {
                connect(m, &ImageProxyModel::loadingChanged, this, &SlideModel::slotSourceModelLoadingChanged);
            } else {
                // In case it loads immediately
                ++m_loaded;
                addSourceModel(m);
            }
        }
    }

    if (!added.empty()) {
        m_loading = m_loaded != m_models.size();
        if (!m_loading) {
            Q_EMIT done();
        }
    }

    return added;
}

QString SlideModel::removeDir(const QString &_dir)
{
    const QString dir = _dir.endsWith(QDir::separator()) ? _dir : _dir + QDir::separator();

    if (!m_models.contains(dir)) {
        return QString();
    }

    auto *m = m_models.take(dir);

    m_loaded--;
    removeSourceModel(m);
    m->deleteLater();

    return dir;
}

void SlideModel::setSlidePaths(const QStringList &slidePaths)
{
    const auto models = sourceModels();

    for (const auto k : std::as_const(m_models)) {
        if (models.contains(k)) {
            removeSourceModel(k);
        } else {
            // Abort loading
            disconnect(k, nullptr, this, nullptr);
        }
        delete k;
    }

    m_models.clear();
    m_loaded = 0;

    addDirs(slidePaths);
}

void SlideModel::setUncheckedSlides(const QStringList &uncheckedSlides)
{
    m_checkedTable.clear();

    for (const QString &p : uncheckedSlides) {
        m_checkedTable[p] = false;
    }
}

QBindable<bool> SlideModel::loading() const
{
    return &m_loading;
}

void SlideModel::slotSourceModelLoadingChanged()
{
    auto m = static_cast<ImageProxyModel *>(sender());
    disconnect(m, &ImageProxyModel::loadingChanged, this, nullptr);
    addSourceModel(m);

    if (++m_loaded == m_models.size()) {
        m_loading = false;
        Q_EMIT done();
    }
}
