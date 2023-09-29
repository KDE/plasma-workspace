/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QJSValue>
#include <QWeakPointer>

#include "containment.h"

class PanelView;
class ShellCorona;

namespace WorkspaceScripting
{
class Panel : public Containment
{
    Q_OBJECT
    Q_PROPERTY(QStringList configKeys READ configKeys)
    Q_PROPERTY(QStringList configGroups READ configGroups)
    Q_PROPERTY(QStringList currentConfigGroup WRITE setCurrentConfigGroup READ currentConfigGroup)
    Q_PROPERTY(QStringList globalConfigKeys READ globalConfigKeys)
    Q_PROPERTY(QStringList globalConfigGroups READ globalConfigGroups)

    Q_PROPERTY(QString version READ version)
    Q_PROPERTY(QString type READ type)
    Q_PROPERTY(QString formFactor READ formFactor)
    Q_PROPERTY(QList<int> widgetIds READ widgetIds)
    Q_PROPERTY(int screen READ screen)
    Q_PROPERTY(QString location READ location WRITE setLocation)
    Q_PROPERTY(int id READ id)

    Q_PROPERTY(bool locked READ locked WRITE setLocked)

    // panel properties
    Q_PROPERTY(QString alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(int floating READ floating WRITE setFloating)
    Q_PROPERTY(int offset READ offset WRITE setOffset)
    Q_PROPERTY(int length READ length WRITE setLength)
    Q_PROPERTY(int minimumLength READ minimumLength WRITE setMinimumLength)
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength)
    Q_PROPERTY(int height READ height WRITE setHeight)
    Q_PROPERTY(QString hiding READ hiding WRITE setHiding)

public:
    explicit Panel(Plasma::Containment *containment, ScriptEngine *parent);
    ~Panel() override;

    QString location() const;
    void setLocation(const QString &location);

    QString alignment() const;
    void setAlignment(const QString &alignment);

    int floating() const;
    void setFloating(int newFloating);

    int offset() const;
    void setOffset(int pixels);

    int length() const;
    void setLength(int pixels);

    int minimumLength() const;
    void setMinimumLength(int pixels);

    int maximumLength() const;
    void setMaximumLength(int pixels);

    int height() const;
    void setHeight(int height);

    QString hiding() const;
    void setHiding(const QString &mode);

public Q_SLOTS:
    void remove()
    {
        Containment::remove();
    }
    void showConfigurationInterface()
    {
        Containment::showConfigurationInterface();
    }

private:
    PanelView *panel() const;
    KConfigGroup panelConfig() const;
    KConfigGroup panelConfigDefaults() const;

    ShellCorona *m_corona;
};

}
