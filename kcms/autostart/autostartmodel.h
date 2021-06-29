/*
    SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QDir>

#include <KService>
#include <optional>

struct AutostartEntry;
class QQuickItem;

class AutostartModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit AutostartModel(QObject *parent = nullptr);

    enum Roles {
        Name = Qt::DisplayRole,
        IconName = Qt::DecorationRole,
        Enabled = Qt::UserRole + 1,
        Source,
        FileName,
        OnlyInPlasma,
    };

    enum AutostartEntrySource {
        XdgAutoStart = 0,
        XdgScripts = 1,
        PlasmaShutdown = 2,
        PlasmaEnvScripts = 3,
    };
    Q_ENUM(AutostartEntrySource)

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool reloadEntry(const QModelIndex &index, const QString &fileName);

    Q_INVOKABLE void removeEntry(int row);
    Q_INVOKABLE void editApplication(int row, QQuickItem *context);
    Q_INVOKABLE void addScript(const QUrl &url, AutostartEntrySource kind);
    Q_INVOKABLE void showApplicationDialog(QQuickItem *context);

    void load();

Q_SIGNALS:
    void error(const QString &message);

private:
    void addApplication(const KService::Ptr &service);
    void loadScriptsFromDir(const QString &subDir, AutostartEntrySource kind);
    void insertScriptEntry(int index, const QString &name, const QString &path, AutostartModel::AutostartEntrySource kind);
    static std::optional<AutostartEntry> loadDesktopEntry(const QString &fileName);

    QDir m_xdgConfigPath;
    QDir m_xdgAutoStartPath;
    QVector<AutostartEntry> m_entries;
};

struct AutostartEntry {
    QString name; // Human readable name or script file path. In case of symlinks the target file path
    AutostartModel::AutostartEntrySource source;
    bool enabled;
    QString fileName; // the file backing the entry
    bool onlyInPlasma;
    QString iconName;
};
Q_DECLARE_TYPEINFO(AutostartEntry, Q_MOVABLE_TYPE);
