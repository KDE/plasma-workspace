/*
    SPDX-FileCopyrightText: 2020 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "fontsaasettingsbase.h"
#include "kxftconfig.h"

class FontAASettingsStore;

class FontsAASettings : public FontsAASettingsBase
{
    Q_OBJECT

    Q_PROPERTY(bool exclude READ exclude WRITE setExclude NOTIFY excludeChanged)
    Q_PROPERTY(int excludeFrom READ excludeFrom WRITE setExcludeFrom NOTIFY excludeFromChanged)
    Q_PROPERTY(int excludeTo READ excludeTo WRITE setExcludeTo NOTIFY excludeToChanged)
    Q_PROPERTY(bool antiAliasing READ antiAliasing WRITE setAntiAliasing NOTIFY antiAliasingChanged)
    Q_PROPERTY(int dpi READ dpi WRITE setDpi NOTIFY dpiChanged)
    Q_PROPERTY(KXftConfig::SubPixel::Type subPixel READ subPixel WRITE setSubPixel NOTIFY subPixelChanged)
    Q_PROPERTY(KXftConfig::Hint::Style hinting READ hinting WRITE setHinting NOTIFY hintingChanged)
    Q_PROPERTY(bool isAaImmutable READ isAaImmutable CONSTANT)
    Q_PROPERTY(bool excludeStateProxy READ excludeStateProxy NOTIFY excludeStateProxyChanged)

public:
    FontsAASettings(QObject *parent = nullptr);

    bool exclude() const;
    int excludeFrom() const;
    int excludeTo() const;
    bool antiAliasing() const;
    int dpi() const;
    KXftConfig::SubPixel::Type subPixel() const;
    KXftConfig::Hint::Style hinting() const;
    bool isAaImmutable() const;
    bool excludeStateProxy() const;

    void setExclude(bool exclude);
    void setExcludeFrom(int excludeFrom);
    void setExcludeTo(int excludeTo);
    void setAntiAliasing(bool enabled);
    void setDpi(int dpi);
    void setSubPixel(KXftConfig::SubPixel::Type type);
    void setHinting(KXftConfig::Hint::Style hinting);

Q_SIGNALS:
    void excludeChanged();
    void excludeFromChanged();
    void excludeToChanged();
    void antiAliasingChanged();
    void dpiChanged();
    void subPixelChanged();
    void hintingChanged();
    void aliasingChangeApplied();
    void excludeStateProxyChanged();

private:
    FontAASettingsStore *m_fontAASettingsStore;
    bool m_isAaImmutable = false;
    bool usrSave() override;

    using NotifySignalType = void (FontsAASettings::*)();
    void addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal);
};
