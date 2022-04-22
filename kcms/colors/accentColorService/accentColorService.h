
#pragma once

#include "colorsmodel.h"
#include "colorssettings.h"

#include <kdedmodule.h>

class AccentColorService : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasmashell.accentColor")
public:
    explicit AccentColorService(QObject *parent, const QList<QVariant> &);

public Q_SLOTS:
    void setAccentColor(const QString &color);

private:
    ColorsSettings *m_settings;
    ColorsModel *m_model;
};
