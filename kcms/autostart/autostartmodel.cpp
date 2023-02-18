/*
    SPDX-FileCopyrightText: 2020 Méven Car <meven.car@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autostartmodel.h"
#include "kcm_autostart_debug.h"

#include <KConfigGroup>
#include <KDesktopFile>
#include <KShell>
#include <QDebug>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QStandardPaths>
#include <QWindow>

#include <QDirIterator>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QRegularExpression>

#include <set>

#include <KFileItem>
#include <KFileUtils>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KOpenWithDialog>
#include <KPropertiesDialog>
#include <autostartscriptdesktopfile.h>

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
    KDesktopFile config(fileName);
    const KConfigGroup grp = config.desktopGroup();
    const auto name = config.readName();
    const bool hidden = grp.readEntry("Hidden", false);

    if (hidden) {
        return {};
    }

    const QStringList notShowList = grp.readXdgListEntry("NotShowIn");
    const QStringList onlyShowList = grp.readXdgListEntry("OnlyShowIn");
    const bool enabled = !(notShowList.contains(QLatin1String("KDE")) || (!onlyShowList.isEmpty() && !onlyShowList.contains(QLatin1String("KDE"))));

    if (!enabled) {
        return {};
    }

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

    if (kind == XdgScripts) {
        const QString targetScriptPath = grp.readEntry("Exec");
        const QString targetFileName = QUrl::fromLocalFile(targetScriptPath).fileName();
        const QString targetScriptDir = QFileInfo(targetScriptPath).absoluteDir().path();

        return AutostartEntry{targetFileName, targetScriptDir, kind, enabled, fileName, onlyInPlasma, iconName};
    }

    return AutostartEntry{name, name, kind, enabled, fileName, onlyInPlasma, iconName};
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

    // Needed to add all script entries after application entries
    QVector<AutostartEntry> scriptEntries;
    const auto filesInfo = m_xdgAutoStartPath.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : filesInfo) {
        if (!KDesktopFile::isDesktopFile(fi.fileName())) {
            continue;
        }

        const std::optional<AutostartEntry> entry = loadDesktopEntry(fi.absoluteFilePath());

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
        QString targetFileDir = fi.absoluteDir().path();
        QString targetFilePath = fi.absoluteFilePath();
        QString fileName = QUrl::fromLocalFile(targetFilePath).fileName();
        const bool isSymlink = fi.isSymLink();
        if (isSymlink) {
            targetFilePath = fi.symLinkTarget();
            QFileInfo symLinkTarget(targetFilePath);
            targetFileDir = symLinkTarget.absoluteDir().path();
            fileName = symLinkTarget.fileName();
        }

        m_entries.push_back({fileName, targetFileDir, kind, true, fi.absoluteFilePath(), false, QStringLiteral("dialog-scripts")});
    }
}

int AutostartModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_entries.count();
}

bool AutostartModel::reloadEntry(const QModelIndex &index, const QString &fileName)
{
    if (!checkIndex(index)) {
        return false;
    }

    const std::optional<AutostartEntry> newEntry = loadDesktopEntry(fileName);

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
    case Name:
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
    case TargetFileDirPath:
        return entry.targetFileDirPath;
    }

    return QVariant();
}

void AutostartModel::addApplication(const KService::Ptr &service)
{
    QString desktopPath;
    // It is important to ensure that we make an exact copy of an existing
    // desktop file (if selected) to enable users to override global autostarts.
    // Also see
    // https://bugs.launchpad.net/ubuntu/+source/kde-workspace/+bug/923360
    if (service->desktopEntryName().isEmpty() || service->entryPath().isEmpty()) {
        // create a new desktop file in s_desktopPath
        desktopPath = m_xdgAutoStartPath.filePath(service->name() + QStringLiteral(".desktop"));

        if (QFileInfo::exists(desktopPath)) {
            QUrl baseUrl = QUrl::fromLocalFile(m_xdgAutoStartPath.path());
            QString newName = suggestName(baseUrl, service->name() + QStringLiteral(".desktop"));
            desktopPath = m_xdgAutoStartPath.filePath(newName);
        }

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
        desktopPath = m_xdgAutoStartPath.filePath(service->storageId());

        KDesktopFile desktopFile(service->entryPath());

        if (QFileInfo::exists(desktopPath)) {
            QUrl baseUrl = QUrl::fromLocalFile(m_xdgAutoStartPath.path());
            QString newName = suggestName(baseUrl, service->storageId());
            desktopPath = m_xdgAutoStartPath.filePath(newName);
        }

        // copy original desktop file to new path
        auto newDesktopFile = desktopFile.copyTo(desktopPath);
        newDesktopFile->sync();
    }

    const QString iconName = !service->icon().isEmpty() ? service->icon() : QStringLiteral("dialog-scripts");

    const auto entry = AutostartEntry{service->name(),
                                      service->name(),
                                      AutostartModel::AutostartEntrySource::XdgAutoStart, // .config/autostart load desktop at startup
                                      true,
                                      desktopPath,
                                      false,
                                      iconName};

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

    QFile scriptFile(url.toLocalFile());

    if (!(scriptFile.permissions() & QFile::ExeUser)) {
        Q_EMIT nonExecutableScript(url.toLocalFile(), kind);
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

        // path of the desktop file that is about to be created
        const QString newFilePath = m_xdgAutoStartPath.absoluteFilePath(fileName + QStringLiteral(".desktop"));

        if (QFileInfo::exists(newFilePath)) {
            const QUrl baseUrl = QUrl::fromLocalFile(m_xdgAutoStartPath.path());
            QString newName = suggestName(baseUrl, fileName + QStringLiteral(".desktop"));

            // remove the .desktop part from String
            newName.chop(8);

            AutostartScriptDesktopFile desktopFile(newName, file.filePath());
            insertScriptEntry(lastLoginScript + 1, file.fileName(), file.absoluteDir().path(), desktopFile.fileName(), kind);

        } else {
            AutostartScriptDesktopFile desktopFile(fileName, file.filePath());
            insertScriptEntry(lastLoginScript + 1, file.fileName(), file.absoluteDir().path(), desktopFile.fileName(), kind);
        }

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
                qCWarning(KCM_AUTOSTART_DEBUG) << "Could not add script entry" << theJob->errorString();
                return;
            }
            const QUrl dest = theJob->property("finalUrl").toUrl();
            const QFileInfo destFile(dest.path());
            const QString symLinkFileName = QUrl::fromLocalFile(destFile.symLinkTarget()).fileName();
            const QFileInfo symLinkTarget{destFile.symLinkTarget()};
            const QString symLinkTargetDir = symLinkTarget.absoluteDir().path();
            insertScriptEntry(m_entries.size(), symLinkFileName, symLinkTargetDir, dest.path(), kind);
        });

        job->start();
    } else {
        Q_ASSERT(0);
    }
}

void AutostartModel::insertScriptEntry(int index, const QString &name, const QString &targetFileDirPath, const QString &path, AutostartEntrySource kind)
{
    beginInsertRows(QModelIndex(), index, index);

    AutostartEntry entry = AutostartEntry{name, targetFileDirPath, kind, true, path, false, QStringLiteral("dialog-scripts")};

    m_entries.insert(index, entry);

    endInsertRows();
}

void AutostartModel::removeEntry(int row)
{
    const auto entry = m_entries.at(row);

    KIO::DeleteJob *job = KIO::del(QUrl::fromLocalFile(entry.fileName), KIO::HideProgressInfo);

    connect(job, &KJob::finished, this, [this, row, entry](KJob *theJob) {
        if (theJob->error()) {
            qCWarning(KCM_AUTOSTART_DEBUG) << "Could not remove entry" << theJob->errorString();
            return;
        }

        beginRemoveRows(QModelIndex(), row, row);
        m_entries.remove(row);

        endRemoveRows();
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
    roleNames.insert(TargetFileDirPath, QByteArrayLiteral("targetFileDirPath"));

    return roleNames;
}

void AutostartModel::editApplication(int row, QQuickItem *context)
{
    const QModelIndex idx = index(row, 0);

    const QString fileName = data(idx, AutostartModel::Roles::FileName).toString();
    KFileItem kfi(QUrl::fromLocalFile(fileName));
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
            reloadEntry(idx, dlg->item().localPath());
        }
    });
    dlg->open();
}

void AutostartModel::makeFileExecutable(const QString &fileName)
{
    QFile file(fileName);

    file.setPermissions(file.permissions() | QFile::ExeUser);
}

// Use slightly modified code copied from frameworks KFileUtils because desktop filenames cannot contain '(' or ' '.
QString AutostartModel::makeSuggestedName(const QString &oldName)
{
    QString basename;

    // Extract the original file extension from the filename
    QMimeDatabase db;
    QString nameSuffix = db.suffixForFileName(oldName);

    if (oldName.lastIndexOf(QLatin1Char('.')) == 0) {
        basename = QStringLiteral(".");
        nameSuffix = oldName;
    } else if (nameSuffix.isEmpty()) {
        const int lastDot = oldName.lastIndexOf(QLatin1Char('.'));
        if (lastDot == -1) {
            basename = oldName;
        } else {
            basename = oldName.left(lastDot);
            nameSuffix = oldName.mid(lastDot);
        }
    } else {
        nameSuffix.prepend(QLatin1Char('.'));
        basename = oldName.left(oldName.length() - nameSuffix.length());
    }

    // check if (number) exists at the end of the oldName and increment that number
    const QRegularExpression re(QStringLiteral("_(\\d+)_"));
    QRegularExpressionMatch rmatch;
    oldName.lastIndexOf(re, -1, &rmatch);
    if (rmatch.hasMatch()) {
        const int currentNum = rmatch.captured(1).toInt();
        const QString number = QString::number(currentNum + 1);
        basename.replace(rmatch.capturedStart(1), rmatch.capturedLength(1), number);
    } else {
        // number does not exist, so just append " _1_" to filename
        basename += QLatin1String("_1_");
    }

    return basename + nameSuffix;
}

QString AutostartModel::suggestName(const QUrl &baseURL, const QString &oldName)
{
    QString suggestedName = makeSuggestedName(oldName);

    if (baseURL.isLocalFile()) {
        const QString basePath = baseURL.toLocalFile() + QLatin1Char('/');
        while (QFileInfo::exists(basePath + suggestedName)) {
            suggestedName = makeSuggestedName(suggestedName);
        }
    }

    return suggestedName;
}
