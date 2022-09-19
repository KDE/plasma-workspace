/*
    SPDX-FileCopyrightText: 2020 Benjamin Port <benjamin.port@enioka.com>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "fontsaasettings.h"

#include <KWindowSystem>
#include <QDebug>

namespace
{
bool defaultExclude()
{
    return false;
}

int defaultExcludeFrom()
{
    return 8;
}

int defaultExcludeTo()
{
    return 15;
}

bool defaultAntiAliasing()
{
    return true;
}

KXftConfig::SubPixel::Type defaultSubPixel()
{
    return KXftConfig::SubPixel::Rgb;
}

KXftConfig::Hint::Style defaultHinting()
{
    return KXftConfig::Hint::Slight;
}
}

class FontAASettingsStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool exclude READ exclude WRITE setExclude)
    Q_PROPERTY(int excludeFrom READ excludeFrom WRITE setExcludeFrom)
    Q_PROPERTY(int excludeTo READ excludeTo WRITE setExcludeTo)
    Q_PROPERTY(bool antiAliasing READ antiAliasing WRITE setAntiAliasing)
    Q_PROPERTY(KXftConfig::SubPixel::Type subPixel READ subPixel WRITE setSubPixel)
    Q_PROPERTY(KXftConfig::Hint::Style hinting READ hinting WRITE setHinting)
public:
    FontAASettingsStore(FontsAASettings *parent = nullptr)
        : QObject(parent)
    {
        load();
    }

    bool exclude() const
    {
        return m_exclude;
    }

    void setExclude(bool exclude)
    {
        if (m_exclude != exclude) {
            m_exclude = exclude;
        }
    }

    int excludeFrom() const
    {
        return m_excludeFrom;
    }

    void setExcludeFrom(int excludeFrom)
    {
        if (m_excludeFrom != excludeFrom) {
            m_excludeFrom = excludeFrom;
        }
    }

    int excludeTo() const
    {
        return m_excludeTo;
    }

    void setExcludeTo(int excludeTo)
    {
        if (m_excludeTo != excludeTo) {
            m_excludeTo = excludeTo;
        }
    }

    bool isImmutable() const
    {
        return m_isImmutable;
    }

    bool antiAliasing() const
    {
        return m_antiAliasing;
    }

    void setAntiAliasing(bool antiAliasing)
    {
        if (antiAliasing != m_antiAliasing) {
            m_antiAliasingChanged = true;
            m_antiAliasing = antiAliasing;
        }
    }

    KXftConfig::SubPixel::Type subPixel() const
    {
        return m_subPixel;
    }

    void setSubPixel(KXftConfig::SubPixel::Type subPixel)
    {
        if (m_subPixel != subPixel) {
            m_subPixelChanged = true;
            m_subPixel = subPixel;
        }
    }

    KXftConfig::Hint::Style hinting() const
    {
        return m_hinting;
    }

    void setHinting(KXftConfig::Hint::Style hinting)
    {
        if (m_hinting != hinting) {
            m_hintingChanged = true;
            m_hinting = hinting;
        }
    }

    void save()
    {
        KXftConfig xft;
        KXftConfig::AntiAliasing::State aaState = KXftConfig::AntiAliasing::NotSet;
        if (m_antiAliasingChanged || xft.antiAliasingHasLocalConfig()) {
            aaState = m_antiAliasing ? KXftConfig::AntiAliasing::Enabled : KXftConfig::AntiAliasing::Disabled;
        }
        xft.setAntiAliasing(aaState);

        if (m_exclude) {
            xft.setExcludeRange(m_excludeFrom, m_excludeTo);
        } else {
            xft.setExcludeRange(0, 0);
        }

        if (m_subPixelChanged || xft.subPixelTypeHasLocalConfig()) {
            xft.setSubPixelType(m_subPixel);
        } else {
            xft.setSubPixelType(KXftConfig::SubPixel::NotSet);
        }

        if (m_hintingChanged || xft.hintStyleHasLocalConfig()) {
            xft.setHintStyle(m_hinting);
        } else {
            xft.setHintStyle(KXftConfig::Hint::NotSet);
        }

        // Write to KConfig to sync with krdb
        KSharedConfig::Ptr config = KSharedConfig::openConfig("kdeglobals");
        KConfigGroup grp(config, "General");

        grp.writeEntry("XftSubPixel", KXftConfig::toStr(m_subPixel));

        if (aaState == KXftConfig::AntiAliasing::NotSet) {
            grp.revertToDefault("XftAntialias");
        } else {
            grp.writeEntry("XftAntialias", aaState == KXftConfig::AntiAliasing::Enabled);
        }

        QString hs(KXftConfig::toStr(m_hinting));
        if (hs != grp.readEntry("XftHintStyle")) {
            if (KXftConfig::Hint::NotSet == m_hinting) {
                grp.revertToDefault("XftHintStyle");
            } else {
                grp.writeEntry("XftHintStyle", hs);
            }
        }

        xft.apply();

        m_subPixelChanged = false;
        m_hintingChanged = false;
        m_antiAliasingChanged = false;
    }

    void load()
    {
        double from, to;
        KXftConfig xft;

        if (xft.getExcludeRange(from, to)) {
            setExclude(true);
            setExcludeFrom(from);
            setExcludeTo(to);
        } else {
            setExclude(defaultExclude());
            setExcludeFrom(defaultExcludeFrom());
            setExcludeTo(defaultExcludeTo());
        }

        // sub pixel
        KXftConfig::SubPixel::Type spType = KXftConfig::SubPixel::NotSet;
        xft.getSubPixelType(spType);
        // if it is not set, we have no subpixel hinting
        if (spType == KXftConfig::SubPixel::NotSet) {
            spType = KXftConfig::SubPixel::None;
        }
        setSubPixel(spType);

        // hinting
        KXftConfig::Hint::Style hStyle = KXftConfig::Hint::NotSet;
        xft.getHintStyle(hStyle);
        // if it is not set, we have no hinting
        if (hStyle == KXftConfig::Hint::NotSet) {
            hStyle = KXftConfig::Hint::None;
        }
        setHinting(hStyle);

        KSharedConfig::Ptr config = KSharedConfig::openConfig("kdeglobals");
        KConfigGroup cg(config, "General");
        m_isImmutable = cg.isEntryImmutable("XftAntialias");

        const auto aaState = xft.getAntiAliasing();
        setAntiAliasing(aaState != KXftConfig::AntiAliasing::Disabled);

        m_subPixelChanged = false;
        m_hintingChanged = false;
        m_antiAliasingChanged = false;
    }

private:
    bool m_isImmutable;
    bool m_antiAliasing;
    bool m_antiAliasingChanged;
    KXftConfig::SubPixel::Type m_subPixel;
    bool m_subPixelChanged;
    KXftConfig::Hint::Style m_hinting;
    bool m_hintingChanged;
    bool m_exclude;
    int m_excludeFrom;
    int m_excludeTo;
};

FontsAASettings::FontsAASettings(QObject *parent)
    : FontsAASettingsBase(parent)
    , m_fontAASettingsStore(new FontAASettingsStore(this))
{
    addItemInternal("exclude", defaultExclude(), &FontsAASettings::excludeChanged);
    addItemInternal("excludeFrom", defaultExcludeFrom(), &FontsAASettings::excludeFromChanged);
    addItemInternal("excludeTo", defaultExcludeTo(), &FontsAASettings::excludeToChanged);
    addItemInternal("antiAliasing", defaultAntiAliasing(), &FontsAASettings::antiAliasingChanged);
    addItemInternal("subPixel", defaultSubPixel(), &FontsAASettings::subPixelChanged);
    addItemInternal("hinting", defaultHinting(), &FontsAASettings::hintingChanged);

    connect(this, &FontsAASettings::forceFontDPIWaylandChanged, this, &FontsAASettings::dpiChanged);
    connect(this, &FontsAASettings::forceFontDPIChanged, this, &FontsAASettings::dpiChanged);
}

void FontsAASettings::addItemInternal(const QByteArray &propertyName, const QVariant &defaultValue, NotifySignalType notifySignal)
{
    auto item = new KPropertySkeletonItem(m_fontAASettingsStore, propertyName, defaultValue);
    addItem(item, propertyName);
    item->setNotifyFunction([this, notifySignal] {
        Q_EMIT(this->*notifySignal)();
    });
}

bool FontsAASettings::exclude() const
{
    return findItem("exclude")->property().toBool();
}

void FontsAASettings::setExclude(bool exclude)
{
    findItem("exclude")->setProperty(exclude);
}

int FontsAASettings::excludeFrom() const
{
    return findItem("excludeFrom")->property().toInt();
}

void FontsAASettings::setExcludeFrom(int excludeFrom)
{
    findItem("excludeFrom")->setProperty(excludeFrom);
}

int FontsAASettings::excludeTo() const
{
    return findItem("excludeTo")->property().toInt();
}

void FontsAASettings::setExcludeTo(int excludeTo)
{
    findItem("excludeTo")->setProperty(excludeTo);
}

bool FontsAASettings::antiAliasing() const
{
    return findItem("antiAliasing")->property().toBool();
}

void FontsAASettings::setAntiAliasing(bool enabled)
{
    if (antiAliasing() == enabled) {
        return;
    }

    findItem("antiAliasing")->setProperty(enabled);
    if (!enabled) {
        setSubPixel(KXftConfig::SubPixel::None);
    } else if (subPixel() == KXftConfig::SubPixel::None) {
        setSubPixel(defaultSubPixel());
    }
}

int FontsAASettings::dpi() const
{
    return KWindowSystem::isPlatformWayland() ? forceFontDPIWayland() : forceFontDPI();
}

void FontsAASettings::setDpi(int newDPI)
{
    if (dpi() == newDPI) {
        return;
    }

    if (KWindowSystem::isPlatformWayland()) {
        setForceFontDPIWayland(newDPI);
    } else {
        setForceFontDPI(newDPI);
    }
    Q_EMIT dpiChanged();
}

KXftConfig::SubPixel::Type FontsAASettings::subPixel() const
{
    return findItem("subPixel")->property().value<KXftConfig::SubPixel::Type>();
}

void FontsAASettings::setSubPixel(KXftConfig::SubPixel::Type type)
{
    if (subPixel() == type) {
        return;
    }

    findItem("subPixel")->setProperty(type);
}

KXftConfig::Hint::Style FontsAASettings::hinting() const
{
    return findItem("hinting")->property().value<KXftConfig::Hint::Style>();
}

bool FontsAASettings::isAaImmutable() const
{
    return m_fontAASettingsStore->isImmutable();
}

bool FontsAASettings::excludeStateProxy() const
{
    return false;
}

void FontsAASettings::setHinting(KXftConfig::Hint::Style hinting)
{
    findItem("hinting")->setProperty(hinting);
}

bool FontsAASettings::usrSave()
{
    m_fontAASettingsStore->save();
    return FontsAASettingsBase::usrSave();
}

#include "fontsaasettings.moc"
