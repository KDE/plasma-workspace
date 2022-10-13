/*
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KPluginFactory>
#include <QDBusConnection>
#include <algorithm>
#include <qvector.h>

#include "../../kcms-common_p.h"
#include "colorSchemeService.h"
#include "colorscheme_service_adaptor.h"

#include "colorsapplicator.h"
#include "colorsmodel.h"

K_PLUGIN_CLASS_WITH_JSON(ColorSchemeService, "colorSchemeService.json")

ColorSchemeService::ColorSchemeService(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_settings(new ColorsSettings(this))
    , m_model(new ColorsModel(this))
{
    new ColorSchemeServiceAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/ColorScheme", this);
    dbus.registerService("org.kde.plasmashell.colorScheme");
}

void ColorSchemeService::setColorScheme(QString colorScheme)
{
    m_model->load();
    m_settings->load();
    m_model->setSelectedScheme(m_settings->colorScheme());

    if (m_settings->colorScheme() == colorScheme) {
        // already set
        return;
    }

    QStringList installedSchemes;
    for (int i = 0; i < m_model->rowCount(QModelIndex()); ++i) {
        installedSchemes << m_model->data(m_model->index(i, 0), ColorsModel::SchemeNameRole).toString();
    }
    if (!installedSchemes.contains(colorScheme)) {
        // not found
        return;
    }

    const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(colorScheme));

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                              QStringLiteral("/org/kde/KWin/BlendChanges"),
                                              QStringLiteral("org.kde.KWin.BlendChanges"),
                                              QStringLiteral("start"));
    msg << 300;
    // This is deliberately blocking so that we ensure Kwin has processed the
    // animation start event before we potentially trigger client side changes
    QDBusConnection::sessionBus().call(msg);

    m_model->setSelectedScheme(colorScheme);
    m_settings->setColorScheme(colorScheme);
    applyScheme(path, m_settings->config(), KConfig::Notify);
    m_settings->save();
    notifyKcmChange(GlobalChangeType::PaletteChanged);
}

#include "colorSchemeService.moc"
