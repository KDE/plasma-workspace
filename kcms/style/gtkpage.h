/*
    SPDX-FileCopyrightText: 2020 Mikhail Zolotukhin <zomial@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <QQmlListReference>

#include "gtkthemesmodel.h"

#include "kdegtkconfig_interface.h"

class GtkPage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(GtkThemesModel *gtkThemesModel MEMBER m_gtkThemesModel NOTIFY gtkThemesModelChanged)

public:
    GtkPage(QObject *parent = nullptr);
    ~GtkPage() override;

    Q_INVOKABLE void load();
    void save();
    void defaults();
    bool isDefaults() const;
    bool isSaveNeeded();

public Q_SLOTS:
    bool gtkPreviewAvailable();

    void showGtkPreview();

    void installGtkThemeFromFile(const QUrl &fileUrl);

    void onThemeRemoved();

Q_SIGNALS:
    void gtkThemesModelChanged(GtkThemesModel *model);

    void showErrorMessage(const QString &message);

    void gtkThemeSettingsChanged();

private:
    GtkThemesModel *m_gtkThemesModel;

    OrgKdeGtkConfigInterface m_gtkConfigInterface;
};
