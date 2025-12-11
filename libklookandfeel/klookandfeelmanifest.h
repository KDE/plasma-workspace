/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "klookandfeel_export.h"

#include <KPackage/Package>

#include <QDir>

/**
 * The LookAndFeelManifest type contains information to store in a global theme package.
 */
class KLOOKANDFEEL_EXPORT KLookAndFeelManifest
{
public:
    KLookAndFeelManifest();

    QString name() const;
    void setName(const QString &name);

    QString id() const;
    void setId(const QString &id);

    QString author() const;
    void setAuthor(const QString &author);

    QString email() const;
    void setEmail(const QString &email);

    QString comment() const;
    void setComment(const QString &comment);

    QString website() const;
    void setWebsite(const QString &website);

    QString license() const;
    void setLicense(const QString &license);

    QString widgetStyle() const;
    void setWidgetStyle(const QString &widgetStyle);

    QString colorScheme() const;
    void setColorScheme(const QString &colorScheme);

    QString iconTheme() const;
    void setIconTheme(const QString &iconTheme);

    QString plasmaTheme() const;
    void setPlasmaTheme(const QString &plasmaTheme);

    QString cursorTheme() const;
    void setCursorTheme(const QString &cursorTheme);

    QString windowSwitcher() const;
    void setWindowSwitcher(const QString &windowSwitcher);

    QString desktopSwitcher() const;
    void setDesktopSwitcher(const QString &desktopSwitcher);

    QString decorationLibrary() const;
    void setDecorationLibrary(const QString &decorationLibrary);

    QString decorationTheme() const;
    void setDecorationTheme(const QString &decorationTheme);

    QString desktopLayout() const;
    void setDesktopLayout(const QString &desktopLayout);

    QString preview() const;
    void setPreview(const QString &preview);

    /**
     * Loads the following current system settings: widget style, color scheme, icon theme,
     * plasma theme, cursor theme, window switcher, desktop switcher, decoration library and theme,
     * desktop layout.
     */
    static KLookAndFeelManifest snapshot();

    /**
     * Saves the look and feel package to the disk.
     */
    void write(const QString &filePath);

private:
    void writeMetaData(const QDir &packageDirectory);
    void writeDefaults(const QDir &contentsDirectory);
    void writeLayout(const QDir &contentsDirectory);
    void writePreview(const QDir &contentsDirectory);

    QString m_name;
    QString m_id;
    QString m_author;
    QString m_email;
    QString m_comment;
    QString m_website;
    QString m_license;
    QString m_widgetStyle;
    QString m_colorScheme;
    QString m_iconTheme;
    QString m_plasmaTheme;
    QString m_cursorTheme;
    QString m_windowSwitcher;
    QString m_desktopSwitcher;
    QString m_decorationLibrary;
    QString m_decorationTheme;
    QString m_desktopLayout;
    QString m_preview;
};
