/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

#include <KConfigGroup>
#include <KNotifyConfig>
#include <KSharedConfig>

#include <canberra.h>

#include "debug.h"

using namespace Qt::StringLiterals;

namespace
{

void canberraFinishCallback(ca_context *c, uint32_t id, int error_code, void *userdata)
{
    Q_UNUSED(id);
    Q_UNUSED(userdata);

    QMetaObject::invokeMethod(
        qApp,
        [c, error_code] {
            if (error_code != CA_SUCCESS) {
                qCWarning(PLASMA_STARTUP_SOUND) << "Failed to cancel canberra context for audio notification:" << ca_strerror(error_code);
            }
            ca_context_destroy(c);
            qApp->quit();
        },
        Qt::QueuedConnection);
    // WARNING: do not do anything else here. If you need more logic then put it into a userdata object please.
    // WARNING: best invokeMethod into a QObject to ensure things run on the gui thread, we must not destroy in the callback!
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationName(u"plasmastartupsound"_s);

    // This features a bit of duplication from KNotifications. Why we need to do this stuff manually was never
    // documented, I am left guessing that it is this way so we don't end up talking to plasmashell while it is initing.

    KNotifyConfig notifyConfig(u"plasma_workspace"_s, u"startkde"_s);
    const QString action = notifyConfig.readEntry(u"Action"_s);
    if (action.isEmpty() || !QStringView(action).split('|'_L1).contains(u"Sound")) {
        // no startup sound configured
        return EXIT_SUCCESS;
    }

    const QString soundName = notifyConfig.readEntry(u"Sound"_s);
    if (soundName.isEmpty()) {
        qCWarning(PLASMA_STARTUP_SOUND) << "Audio notification requested, but no sound file provided in notifyrc file, aborting audio notification";
        return EXIT_SUCCESS;
    }

    const auto config = KSharedConfig::openConfig(u"kdeglobals"_s);
    const KConfigGroup group = config->group(u"Sounds"_s);
    const auto soundTheme = group.readEntry("Theme", u"ocean"_s);
    if (!group.readEntry("Enable", true)) {
        qCDebug(PLASMA_STARTUP_SOUND) << "Notification sounds are globally disabled";
        return EXIT_SUCCESS;
    }

    // Legacy implementation. Fallback lookup for a full path within the `$XDG_DATA_LOCATION/sounds` dirs
    QUrl fallbackUrl;
    const auto dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dataLocation : dataLocations) {
        fallbackUrl = QUrl::fromUserInput(soundName, dataLocation + "/sounds"_L1, QUrl::AssumeLocalFile);
        if (fallbackUrl.isLocalFile() && QFileInfo::exists(fallbackUrl.toLocalFile())) {
            break;
        }
        if (!fallbackUrl.isLocalFile() && fallbackUrl.isValid()) {
            break;
        }
        fallbackUrl.clear();
    }

    ca_context *context = nullptr;
    // context dangles a bit. Gets cleaned up in the finish callback!
    if (int ret = ca_context_create(&context); ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP_SOUND) << "Failed to initialize canberra context for startup sound:" << ca_strerror(ret);
        return EXIT_FAILURE;
    }

    // We aren't actually plasmashell, for all intents and purpose we are part of it though from the user perspective.
    if (int ret = ca_context_change_props(context,
                                          CA_PROP_APPLICATION_NAME,
                                          "plasmashell",
                                          CA_PROP_APPLICATION_ID,
                                          "org.kde.plasmashell.desktop",
                                          CA_PROP_APPLICATION_ICON_NAME,
                                          "plasmashell",
                                          nullptr);
        ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP_SOUND) << "Failed to set application properties on canberra context for startup sound:" << ca_strerror(ret);
    }

    ca_proplist *props = nullptr;
    ca_proplist_create(&props);
    const auto proplistDestroy = qScopeGuard([props] {
        ca_proplist_destroy(props);
    });

    const QByteArray soundNameBytes = soundName.toUtf8();
    const QByteArray soundThemeBytes = soundTheme.toUtf8();
    const QByteArray fallbackUrlBytes = QFile::encodeName(fallbackUrl.toLocalFile());

    ca_proplist_sets(props, CA_PROP_EVENT_ID, soundNameBytes.constData());
    ca_proplist_sets(props, CA_PROP_CANBERRA_XDG_THEME_NAME, soundThemeBytes.constData());
    // Fallback to filename
    if (!fallbackUrl.isEmpty()) {
        ca_proplist_sets(props, CA_PROP_MEDIA_FILENAME, fallbackUrlBytes.constData());
    }
    // We only ever play this sound once, no need to cache it.
    ca_proplist_sets(props, CA_PROP_CANBERRA_CACHE_CONTROL, "never");

    if (int ret = ca_context_play_full(context, 0 /* id */, props, canberraFinishCallback, nullptr); ret != CA_SUCCESS) {
        qCWarning(PLASMA_STARTUP_SOUND) << "Failed to play startup sound with canberra:" << ca_strerror(ret);
        return EXIT_FAILURE;
    }

    return a.exec();
}
