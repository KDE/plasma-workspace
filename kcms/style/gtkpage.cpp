/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

        if (!archiveSubitems.contains(QStringLiteral("gtk-2.0")) && archiveSubitems.indexOf(QRegularExpression(QStringLiteral("gtk-3.*"))) == -1) {
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
    m_gtkThemesModel->setSelectedTheme(QStringLiteral("Breeze"));
}

void GtkPage::load()
{
    m_gtkThemesModel->load();
    m_gtkThemesModel->setSelectedTheme(m_gtkConfigInterface.gtkTheme());
}

bool GtkPage::isDefaults() const
{
    return m_gtkThemesModel->selectedTheme() == QLatin1String("Breeze");
}

bool GtkPage::isSaveNeeded()
{
    return m_gtkThemesModel->selectedTheme() != m_gtkConfigInterface.gtkTheme();
}
