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

#pragma once

#include <QAbstractItemModel>
#include <QStandardItemModel>
#include <config-X11.h>

#include <KQuickAddons/ManagedConfigModule>

#include "fontsaasettings.h"
#include "fontssettings.h"

class FontsData;

/**
 * The Desktop/fonts tab in kcontrol.
 */
class KFonts : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(FontsSettings *fontsSettings READ fontsSettings CONSTANT)
    Q_PROPERTY(FontsAASettings *fontsAASettings READ fontsAASettings CONSTANT)
    Q_PROPERTY(QAbstractItemModel *subPixelOptionsModel READ subPixelOptionsModel CONSTANT)
    Q_PROPERTY(int subPixelCurrentIndex READ subPixelCurrentIndex WRITE setSubPixelCurrentIndex NOTIFY subPixelCurrentIndexChanged)
    Q_PROPERTY(QAbstractItemModel *hintingOptionsModel READ hintingOptionsModel CONSTANT)
    Q_PROPERTY(int hintingCurrentIndex READ hintingCurrentIndex WRITE setHintingCurrentIndex NOTIFY hintingCurrentIndexChanged)

public:
    KFonts(QObject *parent, const KPluginMetaData &metaData, const QVariantList &);
    ~KFonts() override;

    FontsSettings *fontsSettings() const;
    FontsAASettings *fontsAASettings() const;

    int subPixelCurrentIndex() const;
    void setHintingCurrentIndex(int idx);
    int hintingCurrentIndex() const;
    void setSubPixelCurrentIndex(int idx);
    QAbstractItemModel *subPixelOptionsModel() const;
    QAbstractItemModel *hintingOptionsModel() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    Q_INVOKABLE void adjustAllFonts();
    Q_INVOKABLE void adjustFont(const QFont &font, const QString &category);

Q_SIGNALS:
    void fontsHaveChanged();
    void hintingCurrentIndexChanged();
    void subPixelCurrentIndexChanged();
    void aliasingChangeApplied();
    void fontDpiSettingsChanged();

private:
    QFont applyFontDiff(const QFont &fnt, const QFont &newFont, int fontDiffFlags);

    FontsData *m_data;
    QStandardItemModel *m_subPixelOptionsModel;
    QStandardItemModel *m_hintingOptionsModel;
};
