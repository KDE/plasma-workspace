/*
    SPDX-FileCopyrightText: 2023 Ismael Asensio <isma.af@gmail.com>
    SPDX-FileCopyrightText: 2025 Sam Crawford <samlkcrawford@pm.me>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "kcm_soundtheme.h"

#include "kcm_soundtheme_debug.h"
#include "soundthemedata.h"

#include <KIO/FileCopyJob>

#include <QtCore>
#include <algorithm>
#include <canberra.h>
#include <memory>
#include <numeric>
#include <ranges>

#include <QCollator>
#include <QDir>
#include <QMimeDatabase>

#include <KConfig>
#include <KConfigGroup>
#include <KJobUiDelegate>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KTar>
#include <KZip>

using namespace Qt::StringLiterals;

/**
 * @brief Opens an archive
 *
 * @param path to the archive
 * @return the archive if opening was successful else a nullptr
 */
static std::unique_ptr<KArchive> openArchive(const QString &path);

/**
 * @brief Determines if the given archive directory is a theme.
 *
 * @param directory to check for
 * @return true if it contains an index.theme/index.deskop else false
 */
static bool isTheme(const KArchiveDirectory *directory);

/**
 * @brief Installs a theme by copying it to the user's local data.
 *
 * @param theme the archive directory that contains the theme files
 * @param name the name of the directory to save the theme to
 * @return true on success else false
 */
static bool installTheme(const KArchiveDirectory *theme, const QString &name);

/**
 * @brief Installs all themes that can be found recursively
 *
 * @param directory the directory that is searched through
 * @return the number of themes installed
 */
static int installAllThemes(const KArchiveDirectory *directory);

K_PLUGIN_FACTORY_WITH_JSON(KCMSoundThemeFactory, "kcm_soundtheme.json", registerPlugin<KCMSoundTheme>(); registerPlugin<SoundThemeData>();)

constexpr QLatin1String FALLBACK_THEME = QLatin1String("freedesktop");

KCMSoundTheme::KCMSoundTheme(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new SoundThemeData(this))
{
    registerSettings(m_data->settings());

    qmlRegisterUncreatableType<SoundThemeSettings *>("org.kde.private.kcms.soundtheme", 1, 0, "Settings", QStringLiteral("SoundTheme settings"));

    connect(m_data->settings(), &SoundThemeSettings::themeChanged, this, &KCMSoundTheme::themeChanged);
    connect(m_data->settings(), &SoundThemeSettings::soundsEnabledChanged, this, &KCMSoundTheme::cancelSound);
}

KCMSoundTheme::~KCMSoundTheme()
{
    if (m_canberraContext) {
        ca_context_destroy(m_canberraContext);
    }
}

SoundThemeSettings *KCMSoundTheme::settings() const
{
    return m_data->settings();
}

int KCMSoundTheme::currentIndex() const
{
    return indexOf(m_data->settings()->theme());
}

int KCMSoundTheme::indexOf(const QString &themeId) const
{
    for (int row = 0; row < m_themes.count(); row++) {
        const auto &theme = m_themes.at(row);
        if (theme->id == themeId) {
            return row;
        }
    }
    return -1;
}

QString KCMSoundTheme::nameFor(const QString &themeId) const
{
    const int index = indexOf(themeId);
    if (index < 0) {
        return themeId;
    }
    return m_themes.at(index)->name;
}

void KCMSoundTheme::load()
{
    KQuickManagedConfigModule::load();
    loadThemes();
}

void KCMSoundTheme::loadThemes()
{
    // Spec-compliant themes are stored in any of the standard locations `.../share/sounds/<themeId>`
    // and must contain a descriptive `index.theme` file. The properties of the themes can be extended
    // in the user-local paths so we need to cascade their description files
    // Reference: http://0pointer.de/public/sound-theme-spec.html

    m_themes.clear();

    const QStringList soundLocations =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("sounds"), QStandardPaths::LocateDirectory);

    QStringList themeIds;
    for (const QString &location : soundLocations) {
        for (const QString &dirName : QDir(location).entryList({}, QDir::AllDirs | QDir::Readable | QDir::NoDotAndDotDot)) {
            if (themeIds.contains(dirName)) {
                continue;
            }
            themeIds << dirName;

            auto *theme = new ThemeInfo(dirName, this);
            if (!theme->isValid || theme->isHidden) {
                delete theme;
                continue;
            }
            // The fallback "freedesktop" theme identifies itself as "Default" with no comment nor translations
            // which can get confused with the system's default theme
            if (theme->id == FALLBACK_THEME) {
                theme->name = i18nc("Name of the fallback \"freedesktop\" sound theme", "FreeDesktop");
                theme->comment = i18n("Fallback sound theme from freedesktop.org");
            }
            m_themes << theme;
        }
    }

    QCollator collator;
    // Sort by theme name, but leave "freedesktop" default at the last position
    std::ranges::sort(m_themes, [&collator](auto *a, auto *b) {
        if (a->id == FALLBACK_THEME) {
            return false;
        }
        if (b->id == FALLBACK_THEME) {
            return true;
        }
        return collator.compare(a->name, b->name) < 0;
    });

    Q_EMIT themesLoaded();
    Q_EMIT themeChanged();
}

ca_context *KCMSoundTheme::canberraContext()
{
    if (!m_canberraContext) {
        int ret = ca_context_create(&m_canberraContext);
        if (ret != CA_SUCCESS) {
            qCWarning(KCM_SOUNDTHEME) << "Failed to initialize canberra context for audio notification:" << ca_strerror(ret);
            m_canberraContext = nullptr;
            return nullptr;
        }

        // clang-format off
        ret = ca_context_change_props(m_canberraContext,
                                      CA_PROP_APPLICATION_NAME, qUtf8Printable(metaData().name()),
                                      CA_PROP_APPLICATION_ID, qUtf8Printable(metaData().pluginId()),
                                      CA_PROP_APPLICATION_ICON_NAME, qUtf8Printable(metaData().iconName()),
                                      nullptr);
        // clang-format on
        if (ret != CA_SUCCESS) {
            qCWarning(KCM_SOUNDTHEME) << "Failed to set application properties on canberra context for audio notification:" << ca_strerror(ret);
        }
    }

    return m_canberraContext;
}

int KCMSoundTheme::playSound(const QString &themeId, const QStringList &soundList)
{
    ca_proplist *props = nullptr;
    ca_proplist_create(&props);
    ca_proplist_sets(props, CA_PROP_CANBERRA_XDG_THEME_NAME, themeId.toLatin1().constData());
    ca_proplist_sets(props, CA_PROP_CANBERRA_CACHE_CONTROL, "volatile");

    // We don't want several previews playing at the same time
    ca_context_cancel(canberraContext(), 0);

    int result = CA_SUCCESS;
    for (const QString &soundName : soundList) {
        ca_proplist_sets(props, CA_PROP_EVENT_ID, soundName.toLatin1().constData());
        result = ca_context_play_full(canberraContext(), 0, props, &ca_finish_callback, this);
        qCDebug(KCM_SOUNDTHEME) << "Try playing sound" << soundName << "for theme" << themeId << ":" << ca_strerror(result);
        if (result == CA_SUCCESS) {
            m_playingTheme = themeId;
            m_playingSound = soundName;
            Q_EMIT playingChanged();
            break;
        }
    }

    ca_proplist_destroy(props);

    return result;
}

void KCMSoundTheme::cancelSound()
{
    ca_context_cancel(canberraContext(), 0);
}

void KCMSoundTheme::installThemeFromFile(const QUrl &url)
{
    if (url.isLocalFile()) {
        installThemeFile(url.toLocalFile());
        return;
    }
    if (m_tempCopyJob) {
        return;
    }

    auto tempFile = std::make_unique<QTemporaryFile>();
    if (!tempFile->open()) {
        Q_EMIT showErrorMessage(i18nc("@info:status", "Unable to create a temporary file."));
        return;
    }

    const QUrl dest = QUrl::fromLocalFile(tempFile->fileName());
    m_tempCopyJob = KIO::file_copy(url, dest, -1, KIO::Overwrite);
    m_tempCopyJob->uiDelegate()->setAutoErrorHandlingEnabled(true);

    connect(m_tempCopyJob, &KIO::FileCopyJob::result, this, [this, temp = std::move(tempFile)](KJob *job) {
        if (job->error() != KJob::NoError) {
            Q_EMIT showErrorMessage(i18nc("@info:status", "Unable to download the sound theme archive: %1", job->errorText()));
            return;
        }
        installThemeFile(temp->fileName());
    });
}

void KCMSoundTheme::installThemeFile(const QString &path)
{
    std::unique_ptr<KArchive> archive = openArchive(path);
    if (!archive) {
        Q_EMIT showErrorMessage(i18nc("@info:status", "The archive could not be opened."));
        return;
    }

    auto showSuccess = [this](int themeCount) {
        Q_EMIT showSuccessMessage(i18ncp("@info:status", "Theme installed successfully.", "Themes installed successfully.", themeCount));
    };
    auto showFailure = [this]() {
        Q_EMIT showErrorMessage(i18nc("@info:status", "The archive does not contain a valid sound theme."));
    };

    const KArchiveDirectory *rootDirectory = archive->directory();
    if (isTheme(rootDirectory)) { // the theme may be a tar bomb
        const QString name = QFileInfo(path).baseName();
        if (installTheme(rootDirectory, name)) {
            showSuccess(1);
        } else {
            showFailure();
        }
    } else {
        int installedCount = installAllThemes(rootDirectory);
        if (installedCount < 1) {
            showFailure();
            archive->close();
            return;
        }
        showSuccess(installedCount);
    }

    load();
    archive->close();
}

static std::unique_ptr<KArchive> openArchive(const QString &path)
{
    std::unique_ptr<KArchive> archive;
    const QMimeType type = QMimeDatabase().mimeTypeForFile(path);
    if (type.inherits(QStringLiteral("application/zip"))) {
        archive = std::make_unique<KZip>(path);
    } else {
        archive = std::make_unique<KTar>(path);
    }

    if (!archive->open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    return archive;
}

static bool isTheme(const KArchiveDirectory *directory)
{
    const QString indexFile = QStringLiteral("index.theme");
    const QString legacyIndexFile = QStringLiteral("index.desktop");
    const QStringList entries = directory->entries();
    return entries.contains(indexFile) || entries.contains(legacyIndexFile);
}

static bool installTheme(const KArchiveDirectory *theme, const QString &name)
{
    const QString localDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (localDataPath.isEmpty()) {
        return false;
    }
    const QString themesDirectory = localDataPath + QStringLiteral("/sounds/");
    return theme->copyTo(themesDirectory + name);
}

static int installAllThemes(const KArchiveDirectory *directory)
{
    auto toEntry = [&directory](const QString &name) {
        return directory->entry(name);
    };

    auto isDirectory = [](const KArchiveEntry *entry) {
        return entry->isDirectory();
    };

    auto toThemes = [](const KArchiveEntry *dir) {
        const KArchiveDirectory *directory = dynamic_cast<const KArchiveDirectory *>(dir);

        if (isTheme(directory)) {
            if (installTheme(directory, directory->name())) {
                return 1;
            }
            return 0;
        } else {
            return installAllThemes(directory);
        }
    };

    namespace rv = std::ranges::views;
    // clang-format off
    auto counts = directory->entries()
        | rv::transform(toEntry)
        | rv::filter(isDirectory)
        | rv::transform(toThemes);
    // clang-format on
    return std::accumulate(counts.begin(), counts.end(), 0);
}

QString KCMSoundTheme::errorString(int errorCode)
{
    return QString::fromUtf8(ca_strerror(errorCode));
}

void KCMSoundTheme::ca_finish_callback(ca_context *c, uint32_t id, int error_code, void *userdata)
{
    Q_UNUSED(c);
    Q_UNUSED(id);
    Q_UNUSED(error_code);
    QMetaObject::invokeMethod(static_cast<KCMSoundTheme *>(userdata), "onPlayingFinished");
}

void KCMSoundTheme::onPlayingFinished()
{
    m_playingTheme = QString();
    m_playingSound = QString();
    Q_EMIT playingChanged();
}

ThemeInfo::ThemeInfo(const QString &themeId, QObject *parent)
    : QObject(parent)
{
    const QStringList themeInfoSources = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("sounds/%1/index.theme").arg(themeId));

    if (themeInfoSources.isEmpty()) {
        return;
    }

    KConfig config = KConfig();
    config.addConfigSources(themeInfoSources);

    KConfigGroup themeGroup = config.group(u"Sound Theme"_s);
    if (!themeGroup.exists()) {
        return;
    }

    id = themeId;
    name = themeGroup.readEntry("Name", themeId);
    comment = themeGroup.readEntry("Comment", {});
    inherits = themeGroup.readEntry("Inherits", QStringList());
    directories = themeGroup.readEntry("Directories", QStringList());
    isHidden = themeGroup.readEntry("Hidden", false);
    example = themeGroup.readEntry("Example", {});

    isValid = true;
}

#include "kcm_soundtheme.moc"

#include "moc_kcm_soundtheme.cpp"
