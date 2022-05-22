/*
    SPDX-FileCopyrightText: 2006-2007 Stephen Leaf <smileaf@gmail.com>
    SPDX-FileCopyrightText: 2008 Montel Laurent <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autostart.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(Autostart, "kcm_autostart.json")

Autostart::Autostart(QObject *parent, const KPluginMetaData &data, const QVariantList &)
    : KQuickAddons::ConfigModule(parent, data)
    , m_model(new AutostartModel(this))
{
    setButtons(Help);

    qmlRegisterUncreatableType<AutostartModel>("org.kde.plasma.kcm.autostart", 1, 0, "AutostartModel", QStringLiteral("Only for enums"));
}

Autostart::~Autostart()
{
}

AutostartModel *Autostart::model() const
{
    return m_model;
}

void Autostart::load()
{
    m_model->load();
}

void Autostart::defaults()
{
}

void Autostart::save()
{
}

#include "autostart.moc"
