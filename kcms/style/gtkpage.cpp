/*
 * Copyright 2020 Mikhail Zolotukhin <zomial@protonmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QDBusPendingCall>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include <KLocalizedString>
#include <KNS3/DownloadDialog>
#include <KTar>

#include "gtkpage.h"

GtkPage::GtkPage(QObject *parent)
    : QObject(parent)
    , m_gtkThemesModel(new GtkThemesModel(this))
    , m_gtkConfigInterface(QStringLiteral("org.kde.GtkConfig"), QStringLiteral("/GtkConfig"), QDBusConnection::sessionBus())
{
    connect(m_gtkThemesModel, &GtkThemesModel::themeRemoved, this, &GtkPage::onThemeRemoved);

    connect(m_gtkThemesModel, &GtkThemesModel::selectedThemeChanged, this, [this]() {
        Q_EMIT gtkThemeSettingsChanged();
    });

    load();
}

GtkPage::~GtkPage() = default;

QString GtkPage::gtkThemeFromConfig()
{
    // FIXME make aysnc but that requires some refactoring on the UI side
    return m_gtkConfigInterface.gtkTheme().value();
}

bool GtkPage::gtkPreviewAvailable()
{
    return !QStandardPaths::findExecutable(QStringLiteral("gtk3_preview"), {CMAKE_INSTALL_FULL_LIBEXECDIR}).isEmpty();
}

void GtkPage::showGtkPreview()
{
    m_gtkConfigInterface.showGtkThemePreview(m_gtkThemesModel->selectedTheme());
}

void GtkPage::onThemeRemoved()
{
    load();
    defaults();
    save();
}

void GtkPage::onGhnsEntriesChanged(const QQmlListReference &changedEnties)
{
    if (changedEnties.count() == 0) {
        return;
    }

    load();
}

void GtkPage::installGtkThemeFromFile(const QUrl &fileUrl)
{
    QString themesInstallDirectoryPath(QDir::homePath() + QStringLiteral("/.themes"));
    QDir::home().mkpath(themesInstallDirectoryPath);
    KTar themeArchive(fileUrl.path());
    themeArchive.open(QIODevice::ReadOnly);

    auto showError = [this, fileUrl]() {
        Q_EMIT showErrorMessage(i18n("%1 is not a valid GTK Theme archive.", fileUrl.fileName()));
    };

    QString firstEntryName = themeArchive.directory()->entries().first();
    const KArchiveEntry *possibleThemeDirectory = themeArchive.directory()->entry(firstEntryName);
    if (possibleThemeDirectory->isDirectory()) {
        const KArchiveDirectory *themeDirectory = static_cast<const KArchiveDirectory *>(possibleThemeDirectory);
        QStringList archiveSubitems = themeDirectory->entries();

        if (!archiveSubitems.contains(QStringLiteral("gtk-2.0")) && archiveSubitems.indexOf(QRegExp("gtk-3.*")) == -1) {
            showError();
            return;
        }
    } else {
        showError();
        return;
    }

    themeArchive.directory()->copyTo(themesInstallDirectoryPath);

    load();
}

void GtkPage::save()
{
    auto call = m_gtkConfigInterface.setGtkTheme(m_gtkThemesModel->selectedTheme());
    // needs to block so "OK" button closing kcmshell still saves properly
    call.waitForFinished();
}

void GtkPage::defaults()
{
    Q_EMIT selectGtkThemeInCombobox(QStringLiteral("Breeze"));
}

void GtkPage::load()
{
    m_gtkThemesModel->load();
    Q_EMIT selectGtkThemeInCombobox(gtkThemeFromConfig());
}
