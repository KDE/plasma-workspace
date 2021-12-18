/*
    SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autostartmodel.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KShell>
#include <QDebug>
#include <QDir>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QSet>
#include <QStandardPaths>
#include <QWindow>

#include <KFileItem>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KOpenWithDialog>
#include <KPropertiesDialog>
#include <autostartscriptdesktopfile.h>

namespace
{

QString XdgAutoStartDesktopFile(const QString &fileName)
{
    return QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("autostart/") + fileName);
}

QString writableXdgAutoStartDesktopFile(const QString &fileName)
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/autostart/") + fileName;
}
}

// FDO user autostart directories are
// .config/autostart which has .desktop files executed by klaunch or systemd, some of which might be scripts

// Then we have Plasma-specific locations which run scripts
// .config/autostart-scripts which has scripts executed by plasma_session (now migrated to .desktop files)
// .config/plasma-workspace/shutdown which has scripts executed by plasma-shutdown
// .config/plasma-workspace/env which has scripts executed by startplasma

// in the case of pre-startup they have to end in .sh
// everywhere else it doesn't matter

// the comment above describes how autostart *currently* works, it is not definitive documentation on how autostart *should* work

// share/autostart shouldn't be an option as this should be reserved for global autostart entries

std::optional<AutostartEntry> AutostartModel::loadDesktopEntry(const QString &fileName)
{
    // KDesktopFile will load the desktop file in cascaded way, while Autostart spec
    // Requires it to be the first file found, so we use absolute path instead.
    auto path = XdgAutoStartDesktopFile(fileName);
    KDesktopFile config(path);
    // Skip NoDisplay=true files.
    if (config.noDisplay()) {
        return {};
    }
    const KConfigGroup grp = config.desktopGroup();
    QString name = config.readName();
    // Leave an option for user to delete invalid desktop files.
    if (name.isEmpty()) {
        name = fileName;
    }

    const bool hidden = grp.readEntry("Hidden", false);
    const QStringList notShowList = grp.readXdgListEntry("NotShowIn");
    const QStringList onlyShowList = grp.readXdgListEntry("OnlyShowIn");
    const bool enabled = !(hidden || notShowList.contains(QLatin1String("KDE")) || (!onlyShowList.isEmpty() && !onlyShowList.contains(QLatin1String("KDE"))));

    const auto lstEntry = grp.readXdgListEntry("OnlyShowIn");
    const bool onlyInPlasma = lstEntry.contains(QLatin1String("KDE"));
    const QString iconName = !config.readIcon().isEmpty() ? config.readIcon() : QStringLiteral("dialog-scripts");
    const auto kind = AutostartScriptDesktopFile::isAutostartScript(config) ? XdgScripts : XdgAutoStart; // .config/autostart load desktop at startup
    const QString tryCommand = grp.readEntry("TryExec");

    // Try to filter out entries that point to nonexistant programs
    // If TryExec is either found in $PATH or is an absolute file path that exists
    // This doesn't detect uninstalled Flatpaks for example though
    if (!tryCommand.isEmpty() && QStandardPaths::findExecutable(tryCommand).isEmpty() && !QFile::exists(tryCommand)) {
        return {};
    }
    const bool isUser = QFileInfo(m_xdgAutoStartPath.filePath(fileName)).isFile();

    return AutostartEntry{name, kind, enabled, fileName, onlyInPlasma, iconName, isUser};
}

AutostartModel::AutostartModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_xdgConfigPath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
    , m_xdgAutoStartPath(m_xdgConfigPath.filePath(QStringLiteral("autostart")))
{
}

void AutostartModel::load()
{
    beginResetModel();

    m_entries.clear();

    // Creates if doesn't already exist
    m_xdgAutoStartPath.mkpath(QStringLiteral("."));

    QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("autostart"), QStandardPaths::LocateDirectory);

    QSet<QString> files;
    for (const auto &path : paths) {
        QDir dir(path);
        if (!dir.exists()) {
            continue;
        }
        for (const auto &fileName : dir.entryList(QStringList() << QStringLiteral("*.desktop"))) {
            if (!KDesktopFile::isDesktopFile(fileName)) {
                continue;
            }
            if (!files.contains(fileName)) {
                files.insert(fileName);
            }
        }
    }

    // Ensure the order is stable.
    QStringList filesList(files.begin(), files.end());
    filesList.sort();
    qDebug() << filesList;
    // Needed to add all script entries after application entries
    QVector<AutostartEntry> scriptEntries;
    for (const auto &file : filesList) {
        const std::optional<AutostartEntry> entry = loadDesktopEntry(file);

        if (!entry) {
            continue;
        }

        if (entry->source == XdgScripts) {
            scriptEntries.push_back(entry.value());
        } else {
            m_entries.push_back(entry.value());
        }
    }

    m_entries.append(scriptEntries);

    loadScriptsFromDir(QStringLiteral("plasma-workspace/env/"), AutostartModel::AutostartEntrySource::PlasmaEnvScripts);

    loadScriptsFromDir(QStringLiteral("plasma-workspace/shutdown/"), AutostartModel::AutostartEntrySource::PlasmaShutdown);

    endResetModel();
}

void AutostartModel::loadScriptsFromDir(const QString &subDir, AutostartModel::AutostartEntrySource kind)
{
    QDir dir(m_xdgConfigPath.filePath(subDir));
    // Creates if doesn't already exist
    dir.mkpath(QStringLiteral("."));

    const auto autostartDirFilesInfo = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : autostartDirFilesInfo) {
        QString fileName = fi.absoluteFilePath();
        const bool isSymlink = fi.isSymLink();
        if (isSymlink) {
            fileName = fi.symLinkTarget();
        }

        m_entries.push_back({fileName, kind, true, fi.absoluteFilePath(), false, QStringLiteral("dialog-scripts"), true});
    }
}

int AutostartModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_entries.count();
}

bool AutostartModel::reloadEntry(const QModelIndex &index)
{
    if (!checkIndex(index)) {
        return false;
    }

    const auto &entry = m_entries[index.row()];
    const std::optional<AutostartEntry> newEntry = loadDesktopEntry(entry.fileName);

    if (!newEntry) {
        return false;
    }

    m_entries.replace(index.row(), newEntry.value());
    Q_EMIT dataChanged(index, index);
    return true;
}

QVariant AutostartModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return QVariant();
    }

    const auto &entry = m_entries.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return entry.name;
    case Enabled:
        return entry.enabled;
    case Source:
        return entry.source;
    case FileName:
        return entry.fileName;
    case OnlyInPlasma:
        return entry.onlyInPlasma;
    case IconName:
        return entry.iconName;
    case IsUser:
        return entry.isUser;
    }

    return QVariant();
}

void AutostartModel::addApplication(const KService::Ptr &service)
{
    QString desktopPath;
    QString desktopFileName;
    // It is important to ensure that we make an exact copy of an existing
    // desktop file (if selected) to enable users to override global autostarts.
    // Also see
    // https://bugs.launchpad.net/ubuntu/+source/kde-workspace/+bug/923360
    if (service->desktopEntryName().isEmpty() || service->entryPath().isEmpty()) {
        // create a new desktop file in s_desktopPath
        desktopFileName = service->name() + QStringLiteral(".desktop");
        desktopPath = m_xdgAutoStartPath.filePath(service->name() + QStringLiteral(".desktop"));

        KDesktopFile desktopFile(desktopPath);
        KConfigGroup kcg = desktopFile.desktopGroup();
        kcg.writeEntry("Name", service->name());
        kcg.writeEntry("Exec", service->exec());
        kcg.writeEntry("Icon", service->icon());
        kcg.writeEntry("Path", "");
        kcg.writeEntry("Terminal", service->terminal() ? "True" : "False");
        kcg.writeEntry("Type", "Application");
        desktopFile.sync();

    } else {
        desktopFileName = service->desktopEntryName() + QStringLiteral(".desktop");
        desktopPath = m_xdgAutoStartPath.filePath(service->storageId());

        QFile::remove(desktopPath);

        // copy original desktop file to new path
        KDesktopFile desktopFile(service->entryPath());
        auto newDeskTopFile = desktopFile.copyTo(desktopPath);
        newDeskTopFile->sync();
    }

    const QString iconName = !service->icon().isEmpty() ? service->icon() : QStringLiteral("dialog-scripts");

    const auto entry = AutostartEntry{service->name(),
                                      AutostartModel::AutostartEntrySource::XdgAutoStart, // .config/autostart load desktop at startup
                                      true,
                                      desktopFileName,
                                      false,
                                      iconName,
                                      true};

    int lastApplication = -1;
    for (const AutostartEntry &e : qAsConst(m_entries)) {
        if (e.source == AutostartModel::AutostartEntrySource::XdgScripts) {
            break;
        }
        ++lastApplication;
    }

    // push before the script items
    const int index = lastApplication + 1;

    beginInsertRows(QModelIndex(), index, index);

    m_entries.insert(index, entry);

    endInsertRows();
}

void AutostartModel::showApplicationDialog(QQuickItem *context)
{
    KOpenWithDialog *owdlg = new KOpenWithDialog();
    owdlg->setAttribute(Qt::WA_DeleteOnClose);

    if (context && context->window()) {
        if (QWindow *actualWindow = QQuickRenderControl::renderWindowFor(context->window())) {
            owdlg->winId(); // so it creates windowHandle
            owdlg->windowHandle()->setTransientParent(actualWindow);
            owdlg->setModal(true);
        }
    }

    connect(owdlg, &QDialog::finished, this, [this, owdlg](int result) {
        if (result != QDialog::Accepted) {
            return;
        }

        const KService::Ptr service = owdlg->service();

        Q_ASSERT(service);
        if (!service) {
            return; // Don't crash if KOpenWith wasn't able to create service.
        }

        addApplication(service);
    });
    owdlg->open();
}

void AutostartModel::addScript(const QUrl &url, AutostartModel::AutostartEntrySource kind)
{
    const QFileInfo file(url.toLocalFile());

    if (!file.isAbsolute()) {
        Q_EMIT error(i18n("\"%1\" is not an absolute url.", url.toLocalFile()));
        return;
    } else if (!file.exists()) {
        Q_EMIT error(i18n("\"%1\" does not exist.", url.toLocalFile()));
        return;
    } else if (!file.isFile()) {
        Q_EMIT error(i18n("\"%1\" is not a file.", url.toLocalFile()));
        return;
    } else if (!file.isReadable()) {
        Q_EMIT error(i18n("\"%1\" is not readable.", url.toLocalFile()));
        return;
    }

    const QString fileName = url.fileName();

    if (kind == AutostartModel::AutostartEntrySource::XdgScripts) {
        int lastLoginScript = -1;
        for (const AutostartEntry &e : qAsConst(m_entries)) {
            if (e.source == AutostartModel::AutostartEntrySource::PlasmaShutdown) {
                break;
            }
            ++lastLoginScript;
        }

        AutostartScriptDesktopFile desktopFile(fileName, file.filePath());
        insertScriptEntry(lastLoginScript + 1, fileName, fileName + QLatin1String(".desktop"), kind);
    } else if (kind == AutostartModel::AutostartEntrySource::PlasmaShutdown) {
        const QUrl destinationScript = QUrl::fromLocalFile(QDir(m_xdgConfigPath.filePath(QStringLiteral("plasma-workspace/shutdown/"))).filePath(fileName));
        KIO::CopyJob *job = KIO::link(url, destinationScript, KIO::HideProgressInfo);
        job->setAutoRename(true);
        job->setProperty("finalUrl", destinationScript);

        connect(job, &KIO::CopyJob::renamed, this, [](KIO::Job *job, const QUrl &from, const QUrl &to) {
            Q_UNUSED(from)
            // in case the destination filename had to be renamed
            job->setProperty("finalUrl", to);
        });

        connect(job, &KJob::finished, this, [this, url, kind](KJob *theJob) {
            if (theJob->error()) {
                qWarning() << "Could not add script entry" << theJob->errorString();
                return;
            }
            const QUrl dest = theJob->property("finalUrl").toUrl();
            insertScriptEntry(m_entries.size(), dest.fileName(), dest.path(), kind);
        });

        job->start();
    } else {
        Q_ASSERT(0);
    }
}

void AutostartModel::insertScriptEntry(int index, const QString &name, const QString &path, AutostartEntrySource kind)
{
    beginInsertRows(QModelIndex(), index, index);

    AutostartEntry entry = AutostartEntry{name, kind, true, path, false, QStringLiteral("dialog-scripts"), true};

    m_entries.insert(index, entry);

    endInsertRows();
}

void AutostartModel::removeEntry(int row)
{
    const auto entry = m_entries.at(row);
    QString filePath;
    filePath = m_xdgAutoStartPath.filePath(entry.fileName);
    KIO::DeleteJob *job = KIO::del(QUrl::fromLocalFile(filePath), KIO::HideProgressInfo);

    connect(job, &KJob::finished, this, [this, row, entry](KJob *theJob) {
        if (theJob->error()) {
            qWarning() << "Could not remove entry" << theJob->errorString();
            return;
        }

        // If there's a system file available, reload the system entry again.
        if (entry.source == XdgAutoStart && !XdgAutoStartDesktopFile(entry.fileName).isEmpty()) {
            reloadEntry(index(row, 0));
        } else {
            beginRemoveRows(QModelIndex(), row, row);
            m_entries.remove(row);

            endRemoveRows();
        }
    });

    job->start();
}

QHash<int, QByteArray> AutostartModel::roleNames() const
{
    QHash<int, QByteArray> roleNames = QAbstractListModel::roleNames();

    roleNames.insert(Name, QByteArrayLiteral("name"));
    roleNames.insert(Enabled, QByteArrayLiteral("enabled"));
    roleNames.insert(Source, QByteArrayLiteral("source"));
    roleNames.insert(FileName, QByteArrayLiteral("fileName"));
    roleNames.insert(OnlyInPlasma, QByteArrayLiteral("onlyInPlasma"));
    roleNames.insert(IconName, QByteArrayLiteral("iconName"));
    roleNames.insert(IsUser, QByteArrayLiteral("isUser"));

    return roleNames;
}

void AutostartModel::editApplication(int row, QQuickItem *context)
{
    const QModelIndex idx = index(row, 0);

    const QString fileName = data(idx, AutostartModel::Roles::FileName).toString();
    qDebug() << fileName;
    KFileItem kfi(QUrl::fromLocalFile(XdgAutoStartDesktopFile(fileName)));
    kfi.setDelayedMimeTypes(true);

    KPropertiesDialog *dlg = new KPropertiesDialog(kfi, nullptr);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    if (context && context->window()) {
        if (QWindow *actualWindow = QQuickRenderControl::renderWindowFor(context->window())) {
            dlg->winId(); // so it creates windowHandle
            dlg->windowHandle()->setTransientParent(actualWindow);
            dlg->setModal(true);
        }
    }

    connect(dlg, &QDialog::finished, this, [this, idx, dlg](int result) {
        if (result == QDialog::Accepted) {
            reloadEntry(idx);
        }
    });
    dlg->open();
}

void AutostartModel::toggleApplication(int row)
{
    // Only XDG autostart entry support this.
    const auto entry = m_entries.at(row);
    Q_ASSERT(entry.source == XdgAutoStart);

    // Load the old configuration.
    const KDesktopFile config(XdgAutoStartDesktopFile(entry.fileName));
    bool newEnabled = !entry.enabled;
    const KConfigGroup grp = config.desktopGroup();

    QString desktopPath = writableXdgAutoStartDesktopFile(entry.fileName);
    // Update entries are relevant to enable/disable to the entries.
    KDesktopFile *newDesktopFile = config.copyTo(desktopPath);
    KConfigGroup newGroup = newDesktopFile->desktopGroup();
    // We will try to preserve NotShowIn and OnlyShowIn if possible.
    if (newEnabled) {
        newGroup.writeEntry("Hidden", false);
        QStringList notShowList = grp.readXdgListEntry("NotShowIn");
        if (notShowList.removeAll("KDE")) {
            newGroup.writeXdgListEntry("NotShowIn", notShowList);
        }
        QStringList onlyShowList = grp.readXdgListEntry("OnlyShowIn");
        if (!onlyShowList.empty() && !onlyShowList.contains("KDE")) {
            onlyShowList.append("KDE");
            newGroup.writeXdgListEntry("OnlyShowIn", onlyShowList);
        }
    } else {
        newGroup.writeEntry("Hidden", true);
    }

    // Sync the new data to disk.
    delete newDesktopFile;
    // Reload entry from disk.
    reloadEntry(index(row, 0));
}
