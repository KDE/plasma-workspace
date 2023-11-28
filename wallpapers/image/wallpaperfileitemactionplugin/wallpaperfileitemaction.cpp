/*
    SPDX-FileCopyrightText: 2022 Julius Zint <julius@zint.sh>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "wallpaperfileitemaction.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QIcon>
#include <QList>
#include <QMenu>
#include <QScopedPointer>
#include <QVariantList>
#include <QWidget>

#include <KLocalizedString>
#include <KPluginFactory>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(WallpaperFileItemAction, "wallpaperfileitemaction.json")

WallpaperFileItemAction::WallpaperFileItemAction(QObject *parent, const QVariantList &)
    : KAbstractFileItemActionPlugin(parent)
{
}

WallpaperFileItemAction::~WallpaperFileItemAction()
{
}

QList<QAction *> WallpaperFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    if (fileItemInfos.urlList().size() > 1) {
        return {};
    }

    const QString filePath = fileItemInfos.urlList().constFirst().toLocalFile();

    auto menu = new QMenu(i18ndc("plasma_wallpaper_org.kde.image", "@action:inmenu", "Set as Wallpaper"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("viewimage")));

    auto desktopAction = new QAction(i18ndc("plasma_wallpaper_org.kde.image", "@action:inmenu Set as Desktop Wallpaper", "Desktop"));
    connect(desktopAction, &QAction::triggered, this, [this, filePath] {
        setAsDesktopBackground(filePath);
    });
    menu->addAction(desktopAction);

    auto lockAction = new QAction(i18ndc("plasma_wallpaper_org.kde.image", "@action:inmenu Set as Lockscreen Wallpaper", "Lockscreen"));
    connect(lockAction, &QAction::triggered, this, [this, filePath] {
        setAsLockscreenBackground(filePath);
    });
    menu->addAction(lockAction);

    auto bothAction = new QAction(i18ndc("plasma_wallpaper_org.kde.image", "@action:inmenu Set as both lockscreen and Desktop Wallpaper", "Both"));
    connect(bothAction, &QAction::triggered, this, [this, filePath] {
        setAsDesktopBackground(filePath);
        setAsLockscreenBackground(filePath);
    });
    menu->addAction(bothAction);

    menu->setParent(parentWidget, Qt::Popup);
    return {menu->menuAction()};
}

void WallpaperFileItemAction::setAsDesktopBackground(const QString &file)
{
    auto script = QString(
                      "const allDesktops = desktopsForActivity(currentActivity());"
                      " for (i=0; i<allDesktops.length; i++) {"
                      "    d = allDesktops[i];"
                      "    d.wallpaperPlugin = \"org.kde.image\";"
                      "    d.currentConfigGroup = Array(\"Wallpaper\", \"org.kde.image\", \"General\");"
                      "    d.writeConfig(\"Image\", \"%1\")"
                      "}")
                      .arg(file);

    auto message = QDBusMessage::createMethodCall(u"org.kde.plasmashell"_s, u"/PlasmaShell"_s, u"org.kde.PlasmaShell"_s, u"evaluateScript"_s);
    message.setArguments(QVariantList() << QVariant(script));
    QDBusPendingCall pendingCall = QDBusConnection::sessionBus().asyncCall(message);
    auto watcher = new QDBusPendingCallWatcher(pendingCall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher]() {
        watcher->deleteLater();
        const QDBusPendingReply<QString> reply = *watcher;
        if (reply.isError()) {
            auto errorMessage = QString(xi18ndc("plasma_wallpaper_org.kde.image",
                                                "@info %1 is the dbus error message",
                                                "An error occurred while attempting to set the Plasma wallpaper:<nl/>%1"))
                                    .arg(reply.error().message());
            qWarning() << errorMessage;
            error(errorMessage);
        }
    });
}

void WallpaperFileItemAction::setAsLockscreenBackground(const QString &file)
{
    KSharedConfigPtr screenLockerConfig = KSharedConfig::openConfig(u"kscreenlockerrc"_s);
    KConfigGroup cfgGroup = screenLockerConfig->group(QString()).group(u"Greeter"_s).group(u"Wallpaper"_s).group(u"org.kde.image"_s).group(u"General"_s);
    if (screenLockerConfig->accessMode() != KConfig::ReadWrite) {
        auto errorMessage = QString(i18nd("plasma_wallpaper_org.kde.image", "An error occurred while attempting to open kscreenlockerrc config file."));
        qWarning() << errorMessage;
        error(errorMessage);
        return;
    }
    cfgGroup.writeEntry("Image", file);
    cfgGroup.writeEntry("PreviewImage", file);
    screenLockerConfig->sync();
}

#include "wallpaperfileitemaction.moc"
