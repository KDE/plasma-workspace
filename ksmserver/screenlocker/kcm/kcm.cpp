/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "kscreensaversettings.h"
#include "ui_kcm.h"
#include "screenlocker_interface.h"
#include <KCModule>
#include <KPluginFactory>
#include <QVBoxLayout>

class ScreenLockerKcmForm : public QWidget, public Ui::ScreenLockerKcmForm
{
    Q_OBJECT
public:
    explicit ScreenLockerKcmForm(QWidget *parent);
};

ScreenLockerKcmForm::ScreenLockerKcmForm(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
}


class ScreenLockerKcm : public KCModule
{
    Q_OBJECT
public:
    explicit ScreenLockerKcm(QWidget *parent = nullptr, const QVariantList& args = QVariantList());

public Q_SLOTS:
    void save() override;
};

ScreenLockerKcm::ScreenLockerKcm(QWidget *parent, const QVariantList &args)
    : KCModule(parent, args)
{
    ScreenLockerKcmForm *ui = new ScreenLockerKcmForm(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(ui);

    addConfig(KScreenSaverSettings::self(), ui);
}

void ScreenLockerKcm::save()
{
    KCModule::save();

    // reconfigure through DBus
    OrgKdeScreensaverInterface interface(QStringLiteral("org.kde.screensaver"),
                                         QStringLiteral("/ScreenSaver"),
                                         QDBusConnection::sessionBus());
    if (interface.isValid()) {
        interface.configure();
    }
}

K_PLUGIN_FACTORY_WITH_JSON(ScreenLockerKcmFactory,
                           "screenlocker.json",
                           registerPlugin<ScreenLockerKcm>();)

#include "kcm.moc"
