/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "predicatesmonitor_p.h"

#include "devicenotifier_debug.h"

#include <QDirIterator>
#include <QUrl>

#include <KConfigGroup>
#include <KDesktopFile>
#include <KDirWatch>
#include <qloggingcategory.h>

PredicatesMonitor::PredicatesMonitor(QObject *parent)
    : QObject(parent)
    , m_dirWatch(new KDirWatch(this))
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Begin initializing predicates monitor";
    const QStringList folders =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("solid/actions"), QStandardPaths::LocateDirectory);

    for (const QString &folder : folders) {
        qCDebug(APPLETS::DEVICENOTIFIER) << "Predicates Monitor: add watched dir: " << folder;
        m_dirWatch->addDir(folder, KDirWatch::WatchFiles);
    }

    updatePredicates(QString());

    connect(m_dirWatch, &KDirWatch::created, this, &PredicatesMonitor::onPredicatesChanged);
    connect(m_dirWatch, &KDirWatch::deleted, this, &PredicatesMonitor::onPredicatesChanged);
    connect(m_dirWatch, &KDirWatch::dirty, this, &PredicatesMonitor::onPredicatesChanged);
    qCDebug(APPLETS::DEVICENOTIFIER) << "initializing predicates monitor ended";
}

PredicatesMonitor::~PredicatesMonitor() = default;

std::shared_ptr<PredicatesMonitor> PredicatesMonitor::instance()
{
    static std::weak_ptr<PredicatesMonitor> s_clip;
    if (s_clip.expired()) {
        std::shared_ptr<PredicatesMonitor> ptr{new PredicatesMonitor};
        s_clip = ptr;
        return ptr;
    }
    return s_clip.lock();
}

const QHash<QString, Solid::Predicate> &PredicatesMonitor::predicates()
{
    return m_predicates;
}

void PredicatesMonitor::onPredicatesChanged(const QString &path)
{
    qCDebug(APPLETS::DEVICENOTIFIER) << "Predicates Monitor: predicates changed";
    updatePredicates(path);
    Q_EMIT predicatesChanged(m_predicates);
}

void PredicatesMonitor::updatePredicates(const QString &path)
{
    Q_UNUSED(path)
    qCDebug(APPLETS::DEVICENOTIFIER) << "Predicates Monitor: Begin updating predicates";
    m_predicates.clear();
    QStringList files;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("solid/actions"), QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        QDirIterator it(dir, {QStringLiteral("*.desktop")});
        while (it.hasNext()) {
            files.prepend(it.next());
        }
    }
    for (const QString &path : std::as_const(files)) {
        KDesktopFile cfg(path);
        const QString string_predicate = cfg.desktopGroup().readEntry("X-KDE-Solid-Predicate");
        m_predicates.insert(QUrl(path).fileName(), Solid::Predicate::fromString(string_predicate));
    }

    qCDebug(APPLETS::DEVICENOTIFIER) << "Predicates Monitor: predicates updated";
}

#include "moc_predicatesmonitor_p.cpp"
