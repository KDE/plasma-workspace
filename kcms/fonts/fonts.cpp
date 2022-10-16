/*
    SPDX-FileCopyrightText: 1997 Mark Donohoe
    SPDX-FileCopyrightText: 1999 Lars Knoll
    SPDX-FileCopyrightText: 2000 Rik Hemsley
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    Ported to kcontrol2:
    SPDX-FileCopyrightText: Geert Jansen

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fonts.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QFontDatabase>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QWindow>

#include <KAcceleratorManager>
#include <KConfig>
#include <KConfigGroup>
#include <KFontChooserDialog>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KWindowSystem>

#include "../kcms-common_p.h"
#include "krdb.h"
#include "kxftconfig.h"
#include "previewimageprovider.h"

#include "fontsdata.h"

/**** DLL Interface ****/
K_PLUGIN_FACTORY_WITH_JSON(KFontsFactory, "kcm_fonts.json", registerPlugin<KFonts>(); registerPlugin<FontsData>();)

/**** KFonts ****/

KFonts::KFonts(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, metaData, args)
    , m_data(new FontsData(this))
    , m_subPixelOptionsModel(new QStandardItemModel(this))
    , m_hintingOptionsModel(new QStandardItemModel(this))
{
    qmlRegisterAnonymousType<QStandardItemModel>("QStandardItemModel", 1);
    qmlRegisterAnonymousType<FontsSettings>("FontsSettings", 1);
    qmlRegisterAnonymousType<FontsAASettings>("FontsAASettings", 1);

    setButtons(Apply | Default | Help);

    for (KXftConfig::SubPixel::Type t :
         {KXftConfig::SubPixel::None, KXftConfig::SubPixel::Rgb, KXftConfig::SubPixel::Bgr, KXftConfig::SubPixel::Vrgb, KXftConfig::SubPixel::Vbgr}) {
        auto item = new QStandardItem(KXftConfig::description(t));
        m_subPixelOptionsModel->appendRow(item);
    }

    for (KXftConfig::Hint::Style s : {KXftConfig::Hint::None, KXftConfig::Hint::Slight, KXftConfig::Hint::Medium, KXftConfig::Hint::Full}) {
        auto item = new QStandardItem(KXftConfig::description(s));
        m_hintingOptionsModel->appendRow(item);
    }
    connect(fontsAASettings(), &FontsAASettings::hintingChanged, this, &KFonts::hintingCurrentIndexChanged);
    connect(fontsAASettings(), &FontsAASettings::subPixelChanged, this, &KFonts::subPixelCurrentIndexChanged);
}

KFonts::~KFonts()
{
}

FontsSettings *KFonts::fontsSettings() const
{
    return m_data->fontsSettings();
}

FontsAASettings *KFonts::fontsAASettings() const
{
    return m_data->fontsAASettings();
}

QAbstractItemModel *KFonts::subPixelOptionsModel() const
{
    return m_subPixelOptionsModel;
}

QAbstractItemModel *KFonts::hintingOptionsModel() const
{
    return m_hintingOptionsModel;
}

void KFonts::load()
{
    // first load all the settings
    ManagedConfigModule::load();

    // Load preview
    // NOTE: This needs to be done AFTER AA settings is loaded
    // otherwise AA settings will be reset in process of loading
    // previews
    engine()->addImageProvider("preview", new PreviewImageProvider(fontsSettings()->font()));

    // KCM expect save state to be false at this point (can be true because if a font setting loaded
    // from the config isn't available on the system, font substitution may take place)
    setNeedsSave(false);
}

void KFonts::save()
{
    auto dpiItem = fontsAASettings()->findItem("forceFontDPI");
    auto dpiWaylandItem = fontsAASettings()->findItem("forceFontDPIWayland");
    auto antiAliasingItem = fontsAASettings()->findItem("antiAliasing");
    Q_ASSERT(dpiItem && dpiWaylandItem && antiAliasingItem);
    if (dpiItem->isSaveNeeded() || dpiWaylandItem->isSaveNeeded() || antiAliasingItem->isSaveNeeded()) {
        Q_EMIT aliasingChangeApplied();
    }

    auto forceFontDPIChanged = dpiItem->isSaveNeeded();

    ManagedConfigModule::save();

#if HAVE_X11
    // if the setting is reset in the module, remove the dpi value,
    // otherwise don't explicitly remove it and leave any possible system-wide value
    if (fontsAASettings()->forceFontDPI() == 0 && forceFontDPIChanged && !KWindowSystem::isPlatformWayland()) {
        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        proc.start("xrdb",
                   QStringList() << "-quiet"
                                 << "-remove"
                                 << "-nocpp");
        if (proc.waitForStarted()) {
            proc.write(QByteArray("Xft.dpi\n"));
            proc.closeWriteChannel();
            proc.waitForFinished();
        }
    }
    QApplication::processEvents();
#endif

    // Notify the world about the font changes
    if (qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        QDBusMessage message = QDBusMessage::createSignal("/KDEPlatformTheme", "org.kde.KDEPlatformTheme", "refreshFonts");
        QDBusConnection::sessionBus().send(message);
    }

    runRdb(KRdbExportXftSettings | KRdbExportGtkTheme);
}

void KFonts::adjustFont(const QFont &font, const QString &category)
{
    QFont selFont = font;
    int ret = KFontChooserDialog::getFont(selFont, KFontChooser::NoDisplayFlags);

    if (ret == QDialog::Accepted) {
        if (category == QLatin1String("font")) {
            fontsSettings()->setFont(selFont);
        } else if (category == QLatin1String("menuFont")) {
            fontsSettings()->setMenuFont(selFont);
        } else if (category == QLatin1String("toolBarFont")) {
            fontsSettings()->setToolBarFont(selFont);
        } else if (category == QLatin1String("activeFont")) {
            fontsSettings()->setActiveFont(selFont);
        } else if (category == QLatin1String("smallestReadableFont")) {
            fontsSettings()->setSmallestReadableFont(selFont);
        } else if (category == QLatin1String("fixed")) {
            fontsSettings()->setFixed(selFont);
        }
    }
    Q_EMIT fontsHaveChanged();
}

void KFonts::adjustAllFonts()
{
    QFont font = fontsSettings()->font();
    KFontChooser::FontDiffFlags fontDiffFlags;
    int ret = KFontChooserDialog::getFontDiff(font, fontDiffFlags, KFontChooser::NoDisplayFlags);

    if (ret == QDialog::Accepted && fontDiffFlags) {
        fontsSettings()->setFont(applyFontDiff(fontsSettings()->font(), font, fontDiffFlags));
        fontsSettings()->setMenuFont(applyFontDiff(fontsSettings()->menuFont(), font, fontDiffFlags));
        fontsSettings()->setToolBarFont(applyFontDiff(fontsSettings()->toolBarFont(), font, fontDiffFlags));
        fontsSettings()->setActiveFont(applyFontDiff(fontsSettings()->activeFont(), font, fontDiffFlags));

        QFont smallestFont = font;
        // Make the small font 2 points smaller than the general font, but only
        // if the general font is 9pt or higher or else the small font would be
        // borderline unreadable. Assume that if the user is making the font
        // tiny, they want a tiny font everywhere.
        const int generalFontPointSize = font.pointSize();
        if (generalFontPointSize >= 9) {
            smallestFont.setPointSize(generalFontPointSize - 2);
        }
        fontsSettings()->setSmallestReadableFont(applyFontDiff(fontsSettings()->smallestReadableFont(), smallestFont, fontDiffFlags));

        const QFont adjustedFont = applyFontDiff(fontsSettings()->fixed(), font, fontDiffFlags);
        if (QFontInfo(adjustedFont).fixedPitch()) {
            fontsSettings()->setFixed(adjustedFont);
        }
    }
}

QFont KFonts::applyFontDiff(const QFont &fnt, const QFont &newFont, int fontDiffFlags)
{
    QFont font(fnt);

    if (fontDiffFlags & KFontChooser::FontDiffSize) {
        font.setPointSizeF(newFont.pointSizeF());
    }
    if ((fontDiffFlags & KFontChooser::FontDiffFamily)) {
        font.setFamily(newFont.family());
    }
    if (fontDiffFlags & KFontChooser::FontDiffStyle) {
        font.setWeight(newFont.weight());
        font.setStyle(newFont.style());
        font.setUnderline(newFont.underline());
        font.setStyleName(newFont.styleName());
    }

    return font;
}

int KFonts::subPixelCurrentIndex() const
{
    return fontsAASettings()->subPixel() - KXftConfig::SubPixel::None;
}

void KFonts::setSubPixelCurrentIndex(int idx)
{
    fontsAASettings()->setSubPixel(static_cast<KXftConfig::SubPixel::Type>(KXftConfig::SubPixel::None + idx));
}

int KFonts::hintingCurrentIndex() const
{
    return fontsAASettings()->hinting() - KXftConfig::Hint::None;
}

void KFonts::setHintingCurrentIndex(int idx)
{
    fontsAASettings()->setHinting(static_cast<KXftConfig::Hint::Style>(KXftConfig::Hint::None + idx));
}

#include "fonts.moc"
