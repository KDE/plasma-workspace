/*
    SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDir>
#include <QFileIconProvider>

#include <KService>

#include <optional>

#include "unit.h"

struct AutostartEntry;
class QQuickItem;

class AutostartModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool usingSystemdBoot READ usingSystemdBoot CONSTANT)

public:
    explicit AutostartModel(QObject *parent = nullptr);
    ~AutostartModel() override;

    enum Roles {
        Name,
        IconName = Qt::DecorationRole,
        Enabled = Qt::UserRole + 1,
        TargetFileDirPath,
        Source,
        FileName,
        OnlyInPlasma,
        SystemdUnit,
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
    Q_INVOKABLE void addScript(const QUrl &url, AutostartModel::AutostartEntrySource kind);
    Q_INVOKABLE void showApplicationDialog(QQuickItem *context);
    Q_INVOKABLE void makeFileExecutable(const QString &fileName);
    bool usingSystemdBoot() const;

    void load();

Q_SIGNALS:
    void error(const QString &message);
    void nonExecutableScript(const QString &fileName, AutostartModel::AutostartEntrySource kind);

private:
    void addApplication(const KService::Ptr &service);
    void loadScriptsFromDir(const QString &subDir, AutostartEntrySource kind);
    void insertScriptEntry(int index, const QString &name, const QString &targetFileDirPath, const QString &path, AutostartModel::AutostartEntrySource kind);
    QString makeSuggestedName(const QString &oldName);
    void sort(int column = 0, Qt::SortOrder order = Qt::AscendingOrder) override;

    /**
     * Sorts the entries.
     *
     * The entries are sorted first by source, then alphabetically by name.
     *
     * @param entries the entries to sort
     * @return the sorted entries
     */
    static QList<AutostartEntry> sortedEntries(const QList<AutostartEntry> &entries);

    QString suggestName(const QUrl &baseUrl, const QString &oldName);
    static std::optional<AutostartEntry> loadDesktopEntry(const QString &fileName);
    QString systemdEscape(const QString &name) const;

#if HAVE_SYSTEMD
    static constexpr bool haveSystemd = true;
#else
    static constexpr bool haveSystemd = false;
#endif

    QDir m_xdgConfigPath;
    QDir m_xdgAutoStartPath;
    QList<AutostartEntry> m_entries;
    QFileIconProvider m_iconProvider;
};

struct AutostartEntry {
    QString name; // Human readable name of file
    QString targetFileDirPath; // Script file Path without filename. In case of symlinks the target file path without filename. In case of application, Just the
                               // name of the application
    AutostartModel::AutostartEntrySource source;
    bool enabled;
    QString fileName; // the file backing the entry
    bool onlyInPlasma;
    QString iconName;
    Unit *systemdUnit = nullptr; // nullptr for PlasmaEnvScripts and PlamsaShutdown
};
Q_DECLARE_TYPEINFO(AutostartEntry, Q_RELOCATABLE_TYPE);
