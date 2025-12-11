/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "klookandfeelmanifest.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <QDBusConnection>
#include <QDBusMessage>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>

KLookAndFeelManifest::KLookAndFeelManifest()
{
}

QString KLookAndFeelManifest::name() const
{
    return m_name;
}

void KLookAndFeelManifest::setName(const QString &name)
{
    m_name = name;
}

QString KLookAndFeelManifest::id() const
{
    return m_id;
}

void KLookAndFeelManifest::setId(const QString &id)
{
    m_id = id;
}

QString KLookAndFeelManifest::author() const
{
    return m_author;
}

void KLookAndFeelManifest::setAuthor(const QString &author)
{
    m_author = author;
}

QString KLookAndFeelManifest::email() const
{
    return m_email;
}

void KLookAndFeelManifest::setEmail(const QString &email)
{
    m_email = email;
}

QString KLookAndFeelManifest::comment() const
{
    return m_comment;
}

void KLookAndFeelManifest::setComment(const QString &comment)
{
    m_comment = comment;
}

QString KLookAndFeelManifest::website() const
{
    return m_website;
}

void KLookAndFeelManifest::setWebsite(const QString &website)
{
    m_website = website;
}

QString KLookAndFeelManifest::license() const
{
    return m_license;
}

void KLookAndFeelManifest::setLicense(const QString &license)
{
    m_license = license;
}

QString KLookAndFeelManifest::widgetStyle() const
{
    return m_widgetStyle;
}

void KLookAndFeelManifest::setWidgetStyle(const QString &widgetStyle)
{
    m_widgetStyle = widgetStyle;
}

QString KLookAndFeelManifest::colorScheme() const
{
    return m_colorScheme;
}

void KLookAndFeelManifest::setColorScheme(const QString &colorScheme)
{
    m_colorScheme = colorScheme;
}

QString KLookAndFeelManifest::iconTheme() const
{
    return m_iconTheme;
}

void KLookAndFeelManifest::setIconTheme(const QString &iconTheme)
{
    m_iconTheme = iconTheme;
}

QString KLookAndFeelManifest::plasmaTheme() const
{
    return m_plasmaTheme;
}

void KLookAndFeelManifest::setPlasmaTheme(const QString &plasmaTheme)
{
    m_plasmaTheme = plasmaTheme;
}

QString KLookAndFeelManifest::cursorTheme() const
{
    return m_cursorTheme;
}

void KLookAndFeelManifest::setCursorTheme(const QString &cursorTheme)
{
    m_cursorTheme = cursorTheme;
}

QString KLookAndFeelManifest::windowSwitcher() const
{
    return m_windowSwitcher;
}

void KLookAndFeelManifest::setWindowSwitcher(const QString &windowSwitcher)
{
    m_windowSwitcher = windowSwitcher;
}

QString KLookAndFeelManifest::desktopSwitcher() const
{
    return m_desktopSwitcher;
}

void KLookAndFeelManifest::setDesktopSwitcher(const QString &desktopSwitcher)
{
    m_desktopSwitcher = desktopSwitcher;
}

QString KLookAndFeelManifest::decorationLibrary() const
{
    return m_decorationLibrary;
}

void KLookAndFeelManifest::setDecorationLibrary(const QString &decorationLibrary)
{
    m_decorationLibrary = decorationLibrary;
}

QString KLookAndFeelManifest::decorationTheme() const
{
    return m_decorationTheme;
}

void KLookAndFeelManifest::setDecorationTheme(const QString &decorationTheme)
{
    m_decorationTheme = decorationTheme;
}

QString KLookAndFeelManifest::desktopLayout() const
{
    return m_desktopLayout;
}

void KLookAndFeelManifest::setDesktopLayout(const QString &desktopLayout)
{
    m_desktopLayout = desktopLayout;
}

QString KLookAndFeelManifest::preview() const
{
    return m_preview;
}

void KLookAndFeelManifest::setPreview(const QString &preview)
{
    m_preview = preview;
}

KLookAndFeelManifest KLookAndFeelManifest::snapshot()
{
    KLookAndFeelManifest manifest;

    KConfigGroup systemCG(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"));
    manifest.setWidgetStyle(systemCG.readEntry("widgetStyle", QStringLiteral("breeze")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General"));
    manifest.setColorScheme(systemCG.readEntry("ColorScheme", QStringLiteral("Breeze")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("Icons"));
    manifest.setIconTheme(systemCG.readEntry("Theme", QStringLiteral("breeze")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Theme"));
    manifest.setPlasmaTheme(systemCG.readEntry("name", QStringLiteral("default")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kcminputrc")), QStringLiteral("Mouse"));
    manifest.setCursorTheme(systemCG.readEntry("cursorTheme", QStringLiteral("breeze_cursors")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("TabBox"));
    manifest.setWindowSwitcher(systemCG.readEntry("LayoutName", QStringLiteral("org.kde.breeze.desktop")));
    manifest.setDesktopSwitcher(systemCG.readEntry("DesktopLayout", QStringLiteral("org.kde.breeze.desktop")));

    systemCG = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("org.kde.kdecoration2"));
    manifest.setDecorationLibrary(systemCG.readEntry("library", QStringLiteral("org.kde.breeze")));
    manifest.setDecorationTheme(systemCG.readEntry("theme", QString()));

    const QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                                QStringLiteral("/PlasmaShell"),
                                                                QStringLiteral("org.kde.PlasmaShell"),
                                                                QStringLiteral("dumpCurrentLayoutJS"));
    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage) {
        manifest.setDesktopLayout(reply.arguments().first().toString());
    }

    return manifest;
}

void KLookAndFeelManifest::write(const QString &filePath)
{
    const QDir packageDirectory(filePath);
    packageDirectory.mkpath(QStringLiteral("."));

    writeMetaData(packageDirectory);

    const QDir contentsDirectory(packageDirectory.absoluteFilePath(QStringLiteral("contents")));
    contentsDirectory.mkpath(QStringLiteral("."));

    writePreview(contentsDirectory);
    writeDefaults(contentsDirectory);
    writeLayout(contentsDirectory);
}

void KLookAndFeelManifest::writeMetaData(const QDir &packageDirectory)
{
    QFile file(packageDirectory.absoluteFilePath(QStringLiteral("metadata.json")));
    if (!file.open(QFile::WriteOnly)) {
        qWarning("Failed to open %s for writing: %s", qPrintable(file.fileName()), qPrintable(file.errorString()));
        return;
    }

    QJsonObject metaData{
        {QLatin1String("KPackageStructure"), QLatin1String("Plasma/LookAndFeel")},
        {QLatin1String("KPlugin"),
         QJsonObject{
             {QLatin1String("Authors"),
              QJsonArray{QJsonObject{
                  {QLatin1String("Name"), author()},
                  {QLatin1String("Email"), email()},
              }}},
             {QLatin1String("Website"), website()},
             {QLatin1String("Description"), comment()},
             {QLatin1String("Id"), id()},
             {QLatin1String("Name"), name()},
             {QLatin1String("License"), license()},
         }},
        {QLatin1String("Keywords"), QLatin1String("Desktop;Workspace;Appearance;Look and Feel;")},
        {QLatin1String("X-Plasma-APIVersion"), QLatin1String("2")},
        {QLatin1String("X-Plasma-MainScript"), QLatin1String("default")},
    };

    file.write(QJsonDocument(metaData).toJson());
}

void KLookAndFeelManifest::writeDefaults(const QDir &contentsDirectory)
{
    KConfig defaultsConfig(contentsDirectory.absoluteFilePath(QStringLiteral("defaults")));

    KConfigGroup defaultsConfigGroup(&defaultsConfig, QStringLiteral("kdeglobals"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("KDE"));
    defaultsConfigGroup.writeEntry("widgetStyle", widgetStyle());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kdeglobals"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("General"));
    defaultsConfigGroup.writeEntry("ColorScheme", colorScheme());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kdeglobals"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("Icons"));
    defaultsConfigGroup.writeEntry("Theme", iconTheme());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("plasmarc"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("Theme"));
    defaultsConfigGroup.writeEntry("name", plasmaTheme());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kcminputrc"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("Mouse"));
    defaultsConfigGroup.writeEntry("cursorTheme", cursorTheme());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kwinrc"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("WindowSwitcher"));
    defaultsConfigGroup.writeEntry("LayoutName", windowSwitcher());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kwinrc"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("DesktopSwitcher"));
    defaultsConfigGroup.writeEntry("LayoutName", desktopSwitcher());

    defaultsConfigGroup = KConfigGroup(&defaultsConfig, QStringLiteral("kwinrc"));
    defaultsConfigGroup = KConfigGroup(&defaultsConfigGroup, QStringLiteral("org.kde.kdecoration2"));
    defaultsConfigGroup.writeEntry("library", decorationLibrary());
    defaultsConfigGroup.writeEntry("theme", decorationTheme());
}

void KLookAndFeelManifest::writeLayout(const QDir &contentsDirectory)
{
    if (desktopLayout().isEmpty()) {
        return;
    }

    const QDir layoutsDirectory(contentsDirectory.absoluteFilePath(QStringLiteral("layouts")));
    layoutsDirectory.mkpath(QStringLiteral("."));

    QFile file(layoutsDirectory.absoluteFilePath(QStringLiteral("org.kde.plasma.desktop-layout.js")));
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Failed to open %s for writing: %s", qPrintable(file.fileName()), qPrintable(file.errorString()));
        return;
    }

    file.write(desktopLayout().toUtf8());
}

void KLookAndFeelManifest::writePreview(const QDir &contentsDirectory)
{
    if (preview().isEmpty()) {
        return;
    }

    QImageReader reader(preview());
    const QImage fullPreview = reader.read();
    if (fullPreview.isNull()) {
        qWarning("Failed to open previw image %s: %s", qPrintable(preview()), qPrintable(reader.errorString()));
        return;
    }

    const QDir previewsDirectory(contentsDirectory.absoluteFilePath(QStringLiteral("previews")));
    previewsDirectory.mkpath(QStringLiteral("."));

    fullPreview.save(previewsDirectory.absoluteFilePath(QStringLiteral("fullscreenpreview.jpg")));

    const QImage smallPreview = fullPreview.scaledToWidth(512, Qt::SmoothTransformation);
    smallPreview.save(previewsDirectory.absoluteFilePath(QStringLiteral("preview.png")));
}
