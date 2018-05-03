#include "softwarerendernotifier.h"
#include <QMenu>
#include <QIcon>
#include <QProcess>
#include <QGuiApplication>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QQuickWindow>

void SoftwareRendererNotifier::notifyIfRelevant()
{
    if (QQuickWindow::sceneGraphBackend() == QLatin1String("software")) {
        auto group = KSharedConfig::openConfig()->group(QStringLiteral("softwarerenderer"));
        bool neverShow = group.readEntry("neverShow", false);
        if (neverShow) {
            return;
        }
        new SoftwareRendererNotifier(qApp);
    }
}

SoftwareRendererNotifier::SoftwareRendererNotifier(QObject *parent)
    : KStatusNotifierItem(parent)
{
    setTitle(i18n("Software Renderer In Use"));
    setToolTipTitle(i18nc("Tooltip telling user their GL drivers are broken", "Software Renderer In Use"));
    setToolTipSubTitle(i18nc("Tooltip telling user their GL drivers are broken", "Rendering may be degraded"));
    setIconByName(QStringLiteral("video-card-inactive"));
    setStatus(KStatusNotifierItem::Active);
    setStandardActionsEnabled(false);

    connect(this, &KStatusNotifierItem::activateRequested, this, []() {
        QProcess::startDetached("kcmshell5 qtquicksettings");
    });

    auto menu = new QMenu; //ownership is transferred in setContextMenu
    auto action = new QAction(i18n("Never show again"));
    connect(action, &QAction::triggered, this, [this]() {
        auto group = KSharedConfig::openConfig()->group(QStringLiteral("softwarerenderer"));
        group.writeEntry("neverShow", true);
        deleteLater();
    });
    menu->addAction(action);
    setContextMenu(menu);
}

SoftwareRendererNotifier::~SoftwareRendererNotifier() = default;

