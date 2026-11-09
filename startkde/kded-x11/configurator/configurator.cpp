/*
    SPDX-FileCopyrightText: 2026 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "configurator.h"
#include "../../../kcms/kcms-common_p.h"

#include <KPluginFactory>
#include <KProcess>

#include <QDBusConnection>
#include <QTemporaryFile>

#include <xcb/xcb.h>
#include <xcb/xcb_cursor.h>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(Configurator, "configurator.json")

Configurator::Configurator(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_kwinrcWatcher(KConfigWatcher::create(KSharedConfig::openConfig(u"kwinrc"_s)))
{
    sync();

    auto bus = QDBusConnection::sessionBus();
    bus.connect(QString(), u"/KGlobalSettings"_s, u"org.kde.KGlobalSettings"_s, u"notifyChange"_s, this, SLOT(onNotifyChange(int, int)));
    bus.connect(QString(), u"/KDEPlatformTheme"_s, u"org.kde.KDEPlatformTheme"_s, u"refreshFonts"_s, this, SLOT(onRefreshFonts()));

    connect(m_kwinrcWatcher.get(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == u"Xwayland"_s && names.contains("Scale"_ba)) {
            sync();
        }
    });
}

void Configurator::onNotifyChange(int type, int arg)
{
    Q_UNUSED(arg)

    if (type == GlobalChangeType::CursorChanged) {
        sync();
    }
}

void Configurator::onRefreshFonts()
{
    sync();
}

void Configurator::sync()
{
    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        qDebug() << "Couldn't open temp file";
        return;
    }

    KConfigGroup generalCfgGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), u"General"_s);

    // Export the Xcursor theme & size settings
    KConfigGroup mousecfg(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), u"Mouse"_s);
    QString theme = mousecfg.readEntry("cursorTheme", QStringLiteral("breeze_cursors"));
    int cursorSize = mousecfg.readEntry("cursorSize", 24);

    KConfig kwinConfig(QStringLiteral("kwinrc"));
    KConfigGroup xwaylandGroup(&kwinConfig, u"Xwayland"_s);
    qreal xwaylandScale = xwaylandGroup.readEntry("Scale", 1.0);
    cursorSize *= xwaylandScale;

    QString contents;
    contents += "Xcursor.theme: "_L1 + theme + u'\n';
    contents += "Xcursor.size: "_L1 + QString::number(cursorSize) + u'\n';

    const int dpi = xwaylandScale * 96;
    contents += "Xft.dpi: "_L1 + QString::number(dpi) + u'\n';
    contents += QLatin1String("Xft.antialias: ");
    if (generalCfgGroup.readEntry("XftAntialias", true))
        contents += QLatin1String("1\n");
    else
        contents += QLatin1String("0\n");

    const QString hintStyle = generalCfgGroup.readEntry("XftHintStyle", QStringLiteral("hintslight"));
    contents += QLatin1String("Xft.hinting: ");
    if (hintStyle.isEmpty())
        contents += QLatin1String("-1\n");
    else {
        if (hintStyle != QLatin1String("hintnone"))
            contents += QLatin1String("1\n");
        else
            contents += QLatin1String("0\n");
        contents += "Xft.hintstyle: "_L1 + hintStyle + u'\n';
    }

    const QString subPixel = generalCfgGroup.readEntry("XftSubPixel", QStringLiteral("rgb"));
    if (!subPixel.isEmpty())
        contents += "Xft.rgba: "_L1 + subPixel + u'\n';

    if (!contents.isEmpty())
        tmpFile.write(contents.toLatin1());

    tmpFile.flush();

    KProcess proc;
#ifndef NDEBUG
    proc << QStringLiteral("xrdb") << QStringLiteral("-merge") << tmpFile.fileName();
#else
    proc << u"xrdb"_s << u"-quiet"_s << u"-merge"_s << tmpFile.fileName();
#endif
    proc.execute();

    xcb_connection_t *connection = xcb_connect(nullptr, nullptr);
    if (!xcb_connection_has_error(connection)) {
        xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

        // Needed for applications that don't set their own cursor.
        xcb_cursor_context_t *context = nullptr;
        if (xcb_cursor_context_new(connection, screen, &context) < 0) {
            qWarning() << "xcb_cursor_context_new() failed";
        } else {
            xcb_cursor_t cursor = xcb_cursor_load_cursor(context, "left_ptr");
            auto cookie = xcb_change_window_attributes_checked(connection, screen->root, XCB_CW_CURSOR, &cursor);
            if (auto error = xcb_request_check(connection, cookie)) {
                qWarning() << "Failed to set root window cursor, error code:" << error->error_code;
                free(error);
            }
            xcb_free_cursor(connection, cursor);
            xcb_cursor_context_free(context);
        }
    } else {
        qWarning() << "xcb_connect() failed";
    }

    xcb_disconnect(connection);
}

#include "configurator.moc"
#include "moc_configurator.cpp"
