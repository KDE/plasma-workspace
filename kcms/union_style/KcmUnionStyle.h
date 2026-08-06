// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

#pragma once

#include <KQuickManagedConfigModule>

#include "unionstyledata.h"

class KCMUnionStyle : public KQuickManagedConfigModule
{
    Q_OBJECT

public:
    KCMUnionStyle(QObject *parent, const KPluginMetaData &data);

    Q_PROPERTY(QString currentStyleId READ currentStyleId WRITE setCurrentStyleId NOTIFY currentStyleIdChanged)
    QString currentStyleId() const;
    void setCurrentStyleId(const QString &newCurrentStyleId);
    Q_SIGNAL void currentStyleIdChanged();

    Q_PROPERTY(UnionSettings *settings READ settings CONSTANT)
    UnionSettings *settings();

    Q_INVOKABLE bool installStyle(const QUrl &url);
    Q_INVOKABLE bool uninstallStyle(const QString &styleId);

    Q_SIGNAL void showError(const QString &message);
    Q_SIGNAL void showInfo(const QString &message);

private:
    QString m_currentStyleId;

    UnionStyleData *m_data;
};
