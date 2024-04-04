// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFileInfo>
#include <QStandardPaths>

#include <KDEDModule>
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>
#include <KService>

#include "decode.h"

using namespace Qt::StringLiterals;

// Proxy class. This in particular makes use of the implicit trailing QDBusMessage feature to get access to the path.
class OrgFreedesktopDBusPropertiesInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    template<typename... Args>
        requires(sizeof...(Args) > 0)
    OrgFreedesktopDBusPropertiesInterface(Args &&...args)
        : QDBusAbstractInterface(std::forward<Args>(args)...)
    {
    }
    ~OrgFreedesktopDBusPropertiesInterface() override = default;
    Q_DISABLE_COPY_MOVE(OrgFreedesktopDBusPropertiesInterface) // prevent deduction from thinking the variadic ctor can copy

Q_SIGNALS: // SIGNALS
    void
    PropertiesChanged(const QString &interface_name, const QVariantMap &changed_properties, const QStringList &invalidated_properties, const QDBusMessage &msg);
};

class OOMNotifierModule : public KDEDModule
{
    Q_OBJECT
public:
    OOMNotifierModule(QObject *parent, const QList<QVariant> & /*args*/)
        : KDEDModule(parent)
    {
        // NOTE: oom1 actually provides a dbus interface but since units can get killed by other actors we'll want to
        // listen to systemd instead.

        connect(&properties,
                &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged,
                this,
                [this](const auto &interface, const auto &changed, [[maybe_unused]] const auto &invalidated, const QDBusMessage &msg) {
                    if (interface != "org.freedesktop.systemd1.Scope"_L1 && interface != "org.freedesktop.systemd1.Service"_L1) {
                        return;
                    }
                    if (changed.value("Result", QString()) != "oom-kill"_L1) {
                        return;
                    }

                    const auto unit = decodeUnitName(QFileInfo(msg.path()).fileName());
                    const auto service = serviceForUnitName(unit);

                    if (service && service->isValid()) {
                        auto notification = KNotification::event(KNotification::Catastrophe,
                                                                 i18nc("@title", "Memory Shortage Avoided"),
                                                                 i18nc("@label",
                                                                       "The process %1 has been terminated by the Linux kernel because the system is low on "
                                                                       "memory. Consider closing unused applications or browser tabs.",
                                                                       service->name()),
                                                                 service->icon(),
                                                                 KNotification::Persistent);
                        auto action = notification->addAction(i18nc("@action", "Restart Application"));
                        connect(action, &KNotificationAction::activated, this, [this, service] {
                            auto job = new KIO::ApplicationLauncherJob(service, this);
                            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
                            job->start();
                        });
                    }else {
                        KNotification::event(KNotification::Catastrophe,
                                             i18nc("@title", "Memory Shortage Avoided"),
                                             i18nc("@label",
                                                   "A process of unit %1 has been terminated by the Linux kernel because the system is low on "
                                                   "memory. Consider closing unused applications or browser tabs.",
                                                   unit),
                                             "edit-bomb-symbolic",
                                             KNotification::Persistent)
                            ->sendEvent();
                    }
                });
    }

private:
    static KService::Ptr serviceForUnitName(const QString &unitName)
    {
        auto serviceName = unitNameToServiceName(unitName);
        if (serviceName.isEmpty()) {
            return {};
        }

        if (auto service = KService::serviceByMenuId(serviceName.toString() + ".desktop"_L1); service) {
            return service;
        }
        if (unitName.endsWith("@autostart.service"_L1)) {
            if (auto file = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, u"autostart/%1.desktop"_s.arg(serviceName)); !file.isEmpty()) {
                if (auto service = new KService(file); service->isValid()) {
                    return KService::Ptr(service);
                }
            }
        }

        return {};
    }

    OrgFreedesktopDBusPropertiesInterface properties{u"org.freedesktop.systemd1"_s,
                                                     u""_s,
                                                     "org.freedesktop.DBus.Properties",
                                                     QDBusConnection::sessionBus(),
                                                     nullptr};
};

K_PLUGIN_CLASS_WITH_JSON(OOMNotifierModule, "oom-notifier.json")

#include "module.moc"
