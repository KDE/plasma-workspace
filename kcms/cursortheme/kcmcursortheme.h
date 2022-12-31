/*
    SPDX-FileCopyrightText: 2003-2007 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2019 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KNSCore/EntryWrapper>
#include <KQuickAddons/ManagedConfigModule>

#include "cursorthemesettings.h"
#include "launchfeedbacksettings.h"

class QQmlListReference;
class QStandardItemModel;
class QTemporaryFile;

class CursorThemeModel;
class SortProxyModel;
class CursorTheme;
class CursorThemeData;

namespace KIO
{
class FileCopyJob;
}

class CursorThemeConfig : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(CursorThemeSettings *cursorThemeSettings READ cursorThemeSettings CONSTANT)
    Q_PROPERTY(LaunchFeedbackSettings *launchFeedbackSettings READ launchFeedbackSettings CONSTANT)
    Q_PROPERTY(bool canInstall READ canInstall WRITE setCanInstall NOTIFY canInstallChanged)
    Q_PROPERTY(bool canResize READ canResize WRITE setCanResize NOTIFY canResizeChanged)
    Q_PROPERTY(bool canConfigure READ canConfigure WRITE setCanConfigure NOTIFY canConfigureChanged)
    Q_PROPERTY(QAbstractItemModel *cursorsModel READ cursorsModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *sizesModel READ sizesModel CONSTANT)

    Q_PROPERTY(bool downloadingFile READ downloadingFile NOTIFY downloadingFileChanged)
    Q_PROPERTY(int preferredSize READ preferredSize WRITE setPreferredSize NOTIFY preferredSizeChanged)

public:
    CursorThemeConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);
    ~CursorThemeConfig() override;

    void load() override;
    void save() override;
    void defaults() override;

    // for QML properties
    CursorThemeSettings *cursorThemeSettings() const;
    LaunchFeedbackSettings *launchFeedbackSettings() const;

    bool canInstall() const;
    void setCanInstall(bool can);

    bool canResize() const;
    void setCanResize(bool can);

    bool canConfigure() const;
    void setCanConfigure(bool can);

    int preferredSize() const;
    void setPreferredSize(int size);

    bool downloadingFile() const;

    QAbstractItemModel *cursorsModel();
    QAbstractItemModel *sizesModel();

    Q_INVOKABLE int cursorSizeIndex(int cursorSize) const;
    Q_INVOKABLE int cursorSizeFromIndex(int index);
    Q_INVOKABLE int cursorThemeIndex(const QString &cursorTheme) const;
    Q_INVOKABLE QString cursorThemeFromIndex(int index) const;

Q_SIGNALS:
    void canInstallChanged();
    void canResizeChanged();
    void canConfigureChanged();
    void downloadingFileChanged();
    void preferredSizeChanged();
    void themeApplied();

    void showSuccessMessage(const QString &message);
    void showInfoMessage(const QString &message);
    void showErrorMessage(const QString &message);

public Q_SLOTS:
    void ghnsEntryChanged(KNSCore::EntryWrapper *entry);
    void installThemeFromFile(const QUrl &url);

private Q_SLOTS:
    /** Updates the size combo box. It loads the size list of the selected cursor
        theme with the corresponding icons and chooses an appropriate entry. It
        enables the combo box and the label if the theme provides more than one
        size, otherwise it disables it. If the size setting is looked in kiosk
        mode, it stays always disabled. */
    void updateSizeComboBox();

private:
    bool isSaveNeeded() const override;
    void installThemeFile(const QString &path);
    bool iconsIsWritable() const;
    void removeThemes();

    CursorThemeModel *m_themeModel;
    SortProxyModel *m_themeProxyModel;
    QStandardItemModel *m_sizesModel;
    CursorThemeData *m_data;

    /** Holds the last size that was chosen by the user. Example: The user chooses
        theme1 which provides the sizes 24 and 36. He chooses 36. preferredSize gets
        set to 36. Now, he switches to theme2 which provides the sizes 30 and 40.
        preferredSize still is 36, so the UI will default to 40, which is next to 36.
        Now, he chooses theme3 which provides the sizes 34 and 44. preferredSize is
        still 36, so the UI defaults to 34. Now the user changes manually to 44. This
        will also change preferredSize. */
    int m_preferredSize;

    bool m_canInstall;
    bool m_canResize;
    bool m_canConfigure;

    std::unique_ptr<QTemporaryFile> m_tempInstallFile;
    QPointer<KIO::FileCopyJob> m_tempCopyJob;
};
