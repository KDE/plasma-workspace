// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

#include "KcmUnionStyle.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <Union/PackageHandler.h>
#include <Union/StyleRegistry.h>

#include "unionsettings.h"

using namespace Qt::StringLiterals;

KCMUnionStyle::KCMUnionStyle(QObject *parent, const KPluginMetaData &data)
    : KQuickManagedConfigModule(parent, data)
    , m_data(new UnionStyleData(this))
{
    connect(m_data->settings(), &UnionSettings::styleChanged, this, &KCMUnionStyle::currentStyleIdChanged);
}

QString KCMUnionStyle::currentStyleId() const
{
    return m_data->settings()->style();
}

void KCMUnionStyle::setCurrentStyleId(const QString &newCurrentStyleId)
{
    m_data->settings()->setStyle(newCurrentStyleId);
}

UnionSettings *KCMUnionStyle::settings()
{
    return m_data->settings();
}

bool KCMUnionStyle::installStyle(const QUrl &url)
{
    qDebug() << url.toString(QUrl::StripTrailingSlash | QUrl::RemoveScheme);
    auto package = Union::StylePackage(std::filesystem::path(url.toString(QUrl::StripTrailingSlash | QUrl::RemoveScheme).toStdString()));
    if (!package.isValid()) {
        switch (package.error()) {
        case Union::StylePackage::Error::NotFound:
            showError(i18nc("@message:error", "Installation failed: The style could not be found."));
            return false;
        case Union::StylePackage::Error::MissingFiles:
        case Union::StylePackage::Error::InvalidMetaData:
            showError(i18nc("@message:error", "Installation failed: The selected folder is not a valid style."));
            return false;
        case Union::StylePackage::Error::UnknownInputType:
            showError(i18nc("@message:error", "Installation failed: The selected style is not supported."));
            return false;
        case Union::StylePackage::Error::None:
            break;
        }
    }

    auto styleName = package.name();

    auto handler = Union::StyleRegistry::instance()->packageHandler();
    auto result = handler->install(package);
    switch (result) {
    case Union::PackageHandler::Error::None:
        break;
    case Union::PackageHandler::Error::AlreadyInstalled:
        showError(i18nc("@message:error", "Installing style %1 failed: The chosen style is already installed.", styleName));
        return false;
    case Union::PackageHandler::Error::FilesystemError:
        showError(i18nc("@message:error", "Installing style %1 failed: A file system error occurred.", styleName));
        return false;
    default:
        showError(i18nc("@message:error", "Installing style %1 failed: An unexpected error occurred.", styleName));
        return false;
    }

    showInfo(i18nc("@message:info", "Successfully installed style %1.", styleName));
    return true;
}

bool KCMUnionStyle::uninstallStyle(const QString &styleId)
{
    auto handler = Union::StyleRegistry::instance()->packageHandler();
    auto package = handler->package(styleId);
    if (!package.isValid()) {
        showError(i18nc("@message:error", "Uninstall failed: The style could not be found."));
        return false;
    }

    auto styleName = package.name();

    auto result = handler->uninstall(package);
    switch (result) {
    case Union::PackageHandler::Error::None:
        break;
    case Union::PackageHandler::Error::NotInstalled:
        showError(i18nc("@message:error", "Uninstalling style %1 failed: The style could not be found.", styleName));
        return false;
    case Union::PackageHandler::Error::FilesystemError:
        showError(i18nc("@message:error", "Uninstalling style %1 failed: A file system error occurred.", styleName));
        return false;
    default:
        showError(i18nc("@message:error", "Uninstalling sing style %1%1 failed: An unexpected error occurred.", styleName, styleName));
        return false;
    }

    showInfo(i18nc("@message:info", "Successfully uninstalled style %1.", styleName));
    return true;
}

K_PLUGIN_CLASS_WITH_JSON(KCMUnionStyle, "kcm_union_style.json")

#include "KcmUnionStyle.moc"
