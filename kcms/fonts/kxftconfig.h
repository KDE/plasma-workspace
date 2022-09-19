#pragma once

/*
   SPDX-FileCopyrightText: 2002 Craig Drummond <craig@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <config-workspace.h>

#ifdef HAVE_FONTCONFIG

#include <QDateTime>
#include <QDomDocument>
#include <QMetaType>
#include <QStringList>

class KXftConfig
{
public:
    struct Item {
        Item(QDomNode &n)
            : node(n)
            , toBeRemoved(false)
        {
        }
        Item()
            : toBeRemoved(false)
        {
        }
        virtual void reset()
        {
            node.clear();
            toBeRemoved = false;
        }
        bool added()
        {
            return node.isNull();
        }

        QDomNode node;

        virtual ~Item()
        {
        }
        bool toBeRemoved;
    };

    struct SubPixel : public Item {
        enum Type {
            NotSet,
            None,
            Rgb,
            Bgr,
            Vrgb,
            Vbgr,
        };

        SubPixel(Type t, QDomNode &n)
            : Item(n)
            , type(t)
        {
        }
        SubPixel(Type t = NotSet)
            : type(t)
        {
        }

        void reset() override
        {
            Item::reset();
            type = NotSet;
        }

        Type type;
    };

    struct Exclude : public Item {
        Exclude(double f, double t, QDomNode &n)
            : Item(n)
            , from(f)
            , to(t)
        {
        }
        Exclude(double f = 0, double t = 0)
            : from(f)
            , to(t)
        {
        }

        void reset() override
        {
            Item::reset();
            from = to = 0;
        }

        double from, to;
    };

    struct Hint : public Item {
        enum Style {
            NotSet,
            None,
            Slight,
            Medium,
            Full,
        };

        Hint(Style s, QDomNode &n)
            : Item(n)
            , style(s)
        {
        }
        Hint(Style s = NotSet)
            : style(s)
        {
        }

        void reset() override
        {
            Item::reset();
            style = NotSet;
        }

        Style style;
    };

    struct Hinting : public Item {
        Hinting(bool s, QDomNode &n)
            : Item(n)
            , set(s)
        {
        }
        Hinting(bool s = true)
            : set(s)
        {
        }

        void reset() override
        {
            Item::reset();
            set = true;
        }

        bool set;
    };

    struct AntiAliasing : public Item {
        enum State {
            NotSet,
            Enabled,
            Disabled,
        };

        AntiAliasing(State s, QDomNode &n)
            : Item(n)
            , state(s)
        {
        }
        AntiAliasing(State s = NotSet)
            : state(s)
        {
        }

        void reset() override
        {
            Item::reset();
            state = NotSet;
        }

        enum State state;
    };

public:
    explicit KXftConfig(const QString &path = {});

    virtual ~KXftConfig();

    bool reset();
    bool apply();
    bool getSubPixelType(SubPixel::Type &type);
    void setSubPixelType(SubPixel::Type type); // SubPixel::None => turn off sub-pixel rendering
    bool getExcludeRange(double &from, double &to);
    void setExcludeRange(double from, double to); // from:0, to:0 => turn off exclude range
    bool getHintStyle(Hint::Style &style);
    void setHintStyle(Hint::Style style);
    void setAntiAliasing(AntiAliasing::State state);
    AntiAliasing::State getAntiAliasing() const;
    bool antiAliasingHasLocalConfig() const;
    bool subPixelTypeHasLocalConfig() const;
    bool hintStyleHasLocalConfig() const;
    bool changed()
    {
        return m_madeChanges;
    }
    static QString description(SubPixel::Type t);
    static const char *toStr(SubPixel::Type t);
    static QString description(Hint::Style s);
    static const char *toStr(Hint::Style s);
    bool aliasingEnabled();

private:
    bool parseConfigFile(const QString &filename);
    void readContents();
    void applySubPixelType();
    void applyHintStyle();
    void applyAntiAliasing();
    void setHinting(bool set);
    void applyHinting();
    void applyExcludeRange(bool pixel);
    QString getConfigFile();

private:
    QStringList m_globalFiles;

    SubPixel m_subPixel;
    Exclude m_excludeRange, m_excludePixelRange;
    Hint m_hint;
    Hinting m_hinting;
    AntiAliasing m_antiAliasing;
    bool m_antiAliasingHasLocalConfig;
    bool m_subPixelHasLocalConfig;
    bool m_hintHasLocalConfig;
    QDomDocument m_doc;
    QString m_file;
    bool m_madeChanges;
    QDateTime m_time;
};

Q_DECLARE_METATYPE(KXftConfig::Hint::Style)
Q_DECLARE_METATYPE(KXftConfig::SubPixel::Type)
#endif
