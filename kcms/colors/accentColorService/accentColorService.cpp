/*
    SPDX-FileCopyrightText: 2022 Tanbir Jishan <tantalising007@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KPluginFactory>
#include <QDBusConnection>

#include "../../kcms-common_p.h"
#include "accentColorService.h"
#include "accentcolor_service_adaptor.h"
#include "colorsapplicator.h"

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(AccentColorService, "accentColorService.json")

AccentColorService::AccentColorService(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_settings(new ColorsSettings(this))
{
    new AccentColorServiceAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(u"/AccentColor"_s, this);
    dbus.registerService(u"org.kde.plasmashell.accentColor"_s);
}

void AccentColorService::setAccentColor(unsigned accentColor)
{
    const QColor color = QColor::fromRgba(accentColor);
    if (!color.isValid()) {
        return;
    }

    m_settings->load();
    if (m_settings->accentColorFromWallpaper()) {
        const QString path =
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(m_settings->colorScheme()));

        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                  QStringLiteral("/org/kde/KWin/BlendChanges"),
                                                  QStringLiteral("org.kde.KWin.BlendChanges"),
                                                  QStringLiteral("start"));
        msg << 300;
        // This is deliberately blocking so that we ensure Kwin has processed the
        // animation start event before we potentially trigger client side changes
        QDBusConnection::sessionBus().call(msg);

        m_settings->setAccentColor(color);
        applyScheme(path, m_settings->config(), KConfig::Notify, color);
        m_settings->save();
        notifyKcmChange(GlobalChangeType::PaletteChanged);
    }
}

#include "accentColorService.moc"

#include "moc_accentColorService.cpp"
