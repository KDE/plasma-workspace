/*
    KCMStyle
    SPDX-FileCopyrightText: 2002 Karol Szwed <gallium@kde.org>
    SPDX-FileCopyrightText: 2002 Daniel Molkentin <molkentin@kde.org>
    SPDX-FileCopyrightText: 2007 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2009 Davide Bettio <davide.bettio@kdemail.net>
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@broulik.de>
    SPDX-FileCopyrightText: 2019 Cyril Rossi <cyril.rossi@enioka.com>

    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>
    SPDX-FileCopyrightText: 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
    SPDX-FileCopyrightText: 2008 Petri Damsten <damu@iki.fi>
    SPDX-FileCopyrightText: 2000 TrollTech AS.

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "kcmstyle.h"
#include "kcm_style_debug.h"

#include "../kcms-common_p.h"
#include "styleconfdialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KToolBar>

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QLibrary>
#include <QMetaEnum>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickWindow>
#include <QStyleFactory>
#include <QWidget>
#include <QWindow>

#include "krdb.h"

#include "kded_interface.h"

#include "previewitem.h"
#include "styledata.h"

K_PLUGIN_FACTORY_WITH_JSON(KCMStyleFactory, "kcm_style.json", registerPlugin<KCMStyle>(); registerPlugin<StyleData>();)

extern "C" {
Q_DECL_EXPORT void kcminit()
{
    uint flags = KRdbExportQtSettings | KRdbExportQtColors | KRdbExportXftSettings | KRdbExportGtkTheme;
    KConfig _config(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
    KConfigGroup config(&_config, "X11");

    // This key is written by the "colors" module.
    bool exportKDEColors = config.readEntry("exportKDEColors", true);
    if (exportKDEColors) {
        flags |= KRdbExportColors;
    }
    runRdb(flags);
}
}

KCMStyle::KCMStyle(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_data(new StyleData(this))
    , m_model(new StylesModel(this))
{
    const char *uri{"org.kde.private.kcms.style"};

    qmlRegisterUncreatableType<KCMStyle>(uri, 1, 0, "KCM", QStringLiteral("Cannot create instances of KCM"));
    qmlRegisterAnonymousType<StyleSettings>(uri, 1);
    qmlRegisterAnonymousType<StylesModel>(uri, 1);
    qmlRegisterType<PreviewItem>(uri, 1, 0, "PreviewItem");

    connect(m_model, &StylesModel::selectedStyleChanged, this, [this](const QString &style) {
        styleSettings()->setWidgetStyle(style);
    });
    connect(styleSettings(), &StyleSettings::widgetStyleChanged, this, [this] {
        m_model->setSelectedStyle(styleSettings()->widgetStyle());
    });
    connect(styleSettings(), &StyleSettings::iconsOnButtonsChanged, this, [this] {
        m_effectsDirty = true;
    });
    connect(styleSettings(), &StyleSettings::iconsInMenusChanged, this, [this] {
        m_effectsDirty = true;
    });

    m_gtkPage = new GtkPage(this);
    connect(m_gtkPage, &GtkPage::gtkThemeSettingsChanged, this, [this]() {
        settingsChanged();
    });
}

KCMStyle::~KCMStyle() = default;

GtkPage *KCMStyle::gtkPage() const
{
    return m_gtkPage;
}

StylesModel *KCMStyle::model() const
{
    return m_model;
}

StyleSettings *KCMStyle::styleSettings() const
{
    return m_data->settings();
}

KCMStyle::ToolBarStyle KCMStyle::mainToolBarStyle() const
{
    return m_mainToolBarStyle;
}

void KCMStyle::setMainToolBarStyle(ToolBarStyle style)
{
    if (m_mainToolBarStyle != style) {
        m_mainToolBarStyle = style;
        Q_EMIT mainToolBarStyleChanged();

        const QMetaEnum toolBarStyleEnum = QMetaEnum::fromType<ToolBarStyle>();
        styleSettings()->setToolButtonStyle(toolBarStyleEnum.valueToKey(m_mainToolBarStyle));
        m_effectsDirty = true;
    }
}

KCMStyle::ToolBarStyle KCMStyle::otherToolBarStyle() const
{
    return m_otherToolBarStyle;
}

void KCMStyle::setOtherToolBarStyle(ToolBarStyle style)
{
    if (m_otherToolBarStyle != style) {
        m_otherToolBarStyle = style;
        Q_EMIT otherToolBarStyleChanged();

        const QMetaEnum toolBarStyleEnum = QMetaEnum::fromType<ToolBarStyle>();
        styleSettings()->setToolButtonStyleOtherToolbars(toolBarStyleEnum.valueToKey(m_otherToolBarStyle));
        m_effectsDirty = true;
    }
}

void KCMStyle::configure(const QString &title, const QString &styleName, QQuickItem *ctx)
{
    if (m_styleConfigDialog) {
        return;
    }

    const QString configPage = m_model->styleConfigPage(styleName);
    if (configPage.isEmpty()) {
        return;
    }

    QLibrary library(QPluginLoader(configPage).fileName());
    if (!library.load()) {
        qCWarning(KCM_STYLE_DEBUG) << "Failed to load style config page" << configPage << library.errorString();
        Q_EMIT showErrorMessage(i18n("There was an error loading the configuration dialog for this style."));
        return;
    }

    auto allocPtr = library.resolve("allocate_kstyle_config");
    if (!allocPtr) {
        qCWarning(KCM_STYLE_DEBUG) << "Failed to resolve allocate_kstyle_config in" << configPage;
        Q_EMIT showErrorMessage(i18n("There was an error loading the configuration dialog for this style."));
        return;
    }

    m_styleConfigDialog = new StyleConfigDialog(nullptr /*this*/, title);
    m_styleConfigDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_styleConfigDialog->setWindowModality(Qt::WindowModal);
    m_styleConfigDialog->winId(); // so it creates windowHandle

    if (ctx && ctx->window()) {
        if (QWindow *actualWindow = QQuickRenderControl::renderWindowFor(ctx->window())) {
            m_styleConfigDialog->windowHandle()->setTransientParent(actualWindow);
        }
    }

    typedef QWidget *(*factoryRoutine)(QWidget * parent);

    // Get the factory, and make the widget.
    factoryRoutine factory = (factoryRoutine)(allocPtr); // Grmbl. So here I am on my
    //"never use C casts" moralizing streak, and I find that one can't go void* -> function ptr
    // even with a reinterpret_cast.

    QWidget *pluginConfig = factory(m_styleConfigDialog.data());

    // Insert it in...
    m_styleConfigDialog->setMainWidget(pluginConfig);

    //..and connect it to the wrapper
    connect(pluginConfig, SIGNAL(changed(bool)), m_styleConfigDialog.data(), SLOT(setDirty(bool)));
    connect(m_styleConfigDialog.data(), SIGNAL(defaults()), pluginConfig, SLOT(defaults()));
    connect(m_styleConfigDialog.data(), SIGNAL(save()), pluginConfig, SLOT(save()));

    connect(m_styleConfigDialog.data(), &QDialog::accepted, this, [this, styleName] {
        if (!m_styleConfigDialog->isDirty()) {
            return;
        }

        // Force re-rendering of the preview, to apply settings
        Q_EMIT styleReconfigured(styleName);

        // For now, ask all KDE apps to recreate their styles to apply the setitngs
        notifyKcmChange(GlobalChangeType::StyleChanged);

        // When user edited a style, assume they want to use it, too
        styleSettings()->setWidgetStyle(styleName);
    });

    m_styleConfigDialog->show();
}

bool KCMStyle::gtkConfigKdedModuleLoaded() const
{
    return m_gtkConfigKdedModuleLoaded;
}

void KCMStyle::checkGtkConfigKdedModuleLoaded()
{
    org::kde::kded5 kdedInterface(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
    auto call = kdedInterface.loadedModules();
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QStringList> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qCWarning(KCM_STYLE_DEBUG) << "Failed to check whether GTK Config KDED module is loaded" << reply.error().message();
            return;
        }

        const bool isLoaded = reply.value().contains(QLatin1String("gtkconfig"));
        if (m_gtkConfigKdedModuleLoaded != isLoaded) {
            m_gtkConfigKdedModuleLoaded = isLoaded;
            Q_EMIT gtkConfigKdedModuleLoadedChanged();
        }
    });
}

void KCMStyle::load()
{
    checkGtkConfigKdedModuleLoaded();

    m_gtkPage->load();

    ManagedConfigModule::load();
    m_model->load();
    m_previousStyle = styleSettings()->widgetStyle();

    loadSettingsToModel();

    m_effectsDirty = false;
}

void KCMStyle::save()
{
    m_gtkPage->save();

    // Check whether the new style can actually be loaded before saving it.
    // Otherwise apps will use the default style despite something else having been written to the config
    bool newStyleLoaded = false;
    if (styleSettings()->widgetStyle() != m_previousStyle) {
        std::unique_ptr<QStyle> newStyle(QStyleFactory::create(styleSettings()->widgetStyle()));
        if (newStyle) {
            newStyleLoaded = true;
            m_previousStyle = styleSettings()->widgetStyle();
        } else {
            const QString styleDisplay = m_model->data(m_model->index(m_model->indexOfStyle(styleSettings()->widgetStyle()), 0), Qt::DisplayRole).toString();
            Q_EMIT showErrorMessage(i18n("Failed to apply selected style '%1'.", styleDisplay));

            // Reset selected style back to current in case of failure
            styleSettings()->setWidgetStyle(m_previousStyle);
        }
    }

    ManagedConfigModule::save();

    // Export the changes we made to qtrc, and update all qt-only
    // applications on the fly, ensuring that we still follow the user's
    // export fonts/colors settings.
    uint flags = KRdbExportQtSettings | KRdbExportGtkTheme;
    KConfig _kconfig(QStringLiteral("kcmdisplayrc"), KConfig::NoGlobals);
    KConfigGroup kconfig(&_kconfig, "X11");
    bool exportKDEColors = kconfig.readEntry("exportKDEColors", true);
    if (exportKDEColors) {
        flags |= KRdbExportColors;
    }
    runRdb(flags);

    // Now allow KDE apps to reconfigure themselves.
    if (newStyleLoaded) {
        notifyKcmChange(GlobalChangeType::StyleChanged);
    }

    if (m_effectsDirty) {
        // This notifies listeners about:
        //  - GraphicEffectsLevel' config entry, (e.g. to set QPlatformTheme::ThemeHint::UiEffects)
        //  - ShowIconsOnPushButtons config entry, (e.g. to set QPlatformTheme::DialogButtonBoxButtonsHaveIcons)
        notifyKcmChange(GlobalChangeType::SettingsChanged, GlobalSettingsCategory::SETTINGS_STYLE);

        // FIXME - Doesn't apply all settings correctly due to bugs in KApplication/KToolbar.
        // Is this ^ still an issue?
        KToolBar::emitToolbarStyleChanged();
    }

    m_effectsDirty = false;
}

void KCMStyle::defaults()
{
    m_gtkPage->defaults();

    // TODO the old code had a fallback chain but do we actually support not having Breeze for Plasma?
    // defaultStyle() -> oxygen -> plastique -> windows -> platinum -> motif

    ManagedConfigModule::defaults();

    loadSettingsToModel();
}

void KCMStyle::loadSettingsToModel()
{
    Q_EMIT styleSettings()->widgetStyleChanged();

    const QMetaEnum toolBarStyleEnum = QMetaEnum::fromType<ToolBarStyle>();
    setMainToolBarStyle(static_cast<ToolBarStyle>(toolBarStyleEnum.keyToValue(qUtf8Printable(styleSettings()->toolButtonStyle()))));
    setOtherToolBarStyle(static_cast<ToolBarStyle>(toolBarStyleEnum.keyToValue(qUtf8Printable(styleSettings()->toolButtonStyleOtherToolbars()))));
}

bool KCMStyle::isDefaults() const
{
    return m_gtkPage->isDefaults();
}

bool KCMStyle::isSaveNeeded() const
{
    return m_gtkPage->isSaveNeeded();
}

#include "kcmstyle.moc"
