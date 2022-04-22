#include <KPluginFactory>
#include <QDBusConnection>

#include "../../kcms-common_p.h"
#include "accentColorService.h"
#include "accentcolor_service_adaptor.h"
#include "colorsapplicator.h"

K_PLUGIN_CLASS_WITH_JSON(AccentColorService, "accentColorService.json")

AccentColorService::AccentColorService(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_settings(new ColorsSettings(this))
    , m_model(new ColorsModel(this))
{
    new AccentColorServiceAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/AccentColor", this);
    dbus.registerService("org.kde.plasmashell.accentColor");
}

void AccentColorService::setAccentColor(const QString &accentColor)
{
    m_settings->load();
    if (QColor::isValidColor(accentColor) && m_settings->accentColorFromWallpaper()) {
        m_model->load();
        m_model->setSelectedScheme(m_settings->colorScheme());
        const QString path =
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes/%1.colors").arg(m_model->selectedScheme()));

        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                  QStringLiteral("/org/kde/KWin/BlendChanges"),
                                                  QStringLiteral("org.kde.KWin.BlendChanges"),
                                                  QStringLiteral("start"));
        msg << 300;
        // This is deliberately blocking so that we ensure Kwin has processed the
        // animation start event before we potentially trigger client side changes
        QDBusConnection::sessionBus().call(msg);

        m_settings->setAccentColor(accentColor);
        m_settings->save();
        applyScheme(path, m_settings->config(), KConfig::Notify);
        notifyKcmChange(GlobalChangeType::PaletteChanged);
    }
}

#include "accentColorService.moc"
