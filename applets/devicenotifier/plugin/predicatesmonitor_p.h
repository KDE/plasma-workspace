/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KDirWatch>
#include <Solid/Predicate>

/**
 * This class get predicates for a given path. Used by ActionControl to create default actions
 */

class PredicatesMonitor : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<PredicatesMonitor> instance();
    ~PredicatesMonitor() override;

    const QHash<QString, Solid::Predicate> &predicates();

private:
    explicit PredicatesMonitor(QObject *parent = nullptr);

    void updatePredicates(const QString &path);

Q_SIGNALS:
    void predicatesChanged(const QHash<QString, Solid::Predicate> &predicates);

private Q_SLOTS:
    void onPredicatesChanged(const QString &path);

private:
    QHash<QString, Solid::Predicate> m_predicates;
    KDirWatch *m_dirWatch;
};
