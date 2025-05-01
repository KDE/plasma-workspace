/*
    SPDX-FileCopyrightText: 2022 Julius Zint <julius@zint.sh>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef WALLPAPERFILEITEMACTION_H
#define WALLPAPERFILEITEMACTION_H

#include <KAbstractFileItemActionPlugin>
#include <KFileItemListProperties>

class QAction;
class QWidget;

class WallpaperFileItemAction : public KAbstractFileItemActionPlugin
{
    Q_OBJECT
public:
    WallpaperFileItemAction(QObject *parent, const QVariantList &args);
    ~WallpaperFileItemAction() override;
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override;

private Q_SLOTS:
    void setAsDesktopBackground(const QString &file);
    void setAsLockscreenBackground(const QString &file);
};

#endif // WALLPAPERFILEITEMACTION_H
