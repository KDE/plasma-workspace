/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <optional>

#include <PlasmaQuick/ConfigView>
#include <PlasmaQuick/ContainmentView>
#include <QFuture>
#include <QPointer>

#include <KConfigWatcher>

#include "config-workspace.h"
#include "panelview.h"

namespace LayerShellQt
{
class Window;
}

class DesktopView : public PlasmaQuick::ContainmentView
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap candidateContainments READ candidateContainmentsGraphicItems NOTIFY candidateContainmentsChanged)

    /**
     * Whether the desktop is used in accent color extraction
     *
     * @note When usedInAccentColor becomes @c true, \Kirigami.ImageColors
     * will be loaded and update the accent color, and \setAccentColor will
     * be called
     */
    Q_PROPERTY(bool usedInAccentColor READ usedInAccentColor NOTIFY usedInAccentColorChanged)

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor RESET resetAccentColor NOTIFY accentColorChanged)

    /**
     * The available screen rect excluding every panel, including autohide ones.
     * This should be used only for computing the area for the desktop zoom out in edit mode
     */
    Q_PROPERTY(QRect strictAvailableScreenRect READ strictAvailableScreenRect NOTIFY strictAvailableScreenRectChanged)

#if PROJECT_VERSION_PATCH >= 80 || PROJECT_VERSION_MINOR >= 80
    Q_PROPERTY(bool showPreviewBanner READ showPreviewBanner NOTIFY showPreviewBannerChanged)
    Q_PROPERTY(QString previewBannerTitle READ previewBannerTitle CONSTANT)
    Q_PROPERTY(QString previewBannerText READ previewBannerText CONSTANT)
#endif

    /**
     * information about the screen in which the panel is in
     */
    Q_PROPERTY(QScreen *screenToFollow READ screenToFollow WRITE setScreenToFollow NOTIFY screenToFollowChanged)

public:
    explicit DesktopView(Plasma::Corona *corona, QScreen *targetScreen = nullptr);
    ~DesktopView() override;

    /*This is different from screen() as is always there, even if the window is
      temporarily outside the screen or if is hidden: only plasmashell will ever
      change this property, unlike QWindow::screen()*/
    void setScreenToFollow(QScreen *screen);
    QScreen *screenToFollow() const;

    void adaptToScreen();
    void showEvent(QShowEvent *) override;

    bool usedInAccentColor() const;

    QColor accentColor() const;
    void setAccentColor(const QColor &);
    void resetAccentColor();

    bool enteredEditMode() const;

#if PROJECT_VERSION_PATCH >= 80 || PROJECT_VERSION_MINOR >= 80
    bool showPreviewBanner() const;
    QString previewBannerTitle() const;
    QString previewBannerText() const;
    Q_INVOKABLE void showPreviewBannerMenu(const QPoint &pos);
#endif

    QRect strictAvailableScreenRect() const;

    QVariantMap candidateContainmentsGraphicItems() const;

    Q_INVOKABLE QString fileFromPackage(const QString &key, const QString &fileName);

protected:
    bool event(QEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

protected Q_SLOTS:
    /**
     * It will be called when the configuration is requested
     */
    void showConfigurationInterface(Plasma::Applet *applet) override;

private Q_SLOTS:
    void slotContainmentChanged();
    void slotScreenChanged(int newId);
    void screenGeometryChanged();

Q_SIGNALS:
    void stayBehindChanged();
    void candidateContainmentsChanged();
    void geometryChanged();
    void usedInAccentColorChanged();
    void accentColorChanged(const QColor &accentColor);
    void screenToFollowChanged(QScreen *screen);
#if PROJECT_VERSION_PATCH >= 80 || PROJECT_VERSION_MINOR >= 80
    void showPreviewBannerChanged();
#endif
    void strictAvailableScreenRectChanged();

private:
    void coronaPackageChanged(const KPackage::Package &package);
    void setAccentColorFromWallpaper(const QColor &accentColor);
    bool handleKRunnerTextInput(QKeyEvent *e);

    std::optional<QColor> m_accentColor;
    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QScreen> m_screenToFollow;
    LayerShellQt::Window *m_layerWindow = nullptr;
    QString m_krunnerText;
    QFuture<QString> m_krunnerFuture;

    // KRunner config
    KConfigWatcher::Ptr m_configWatcher;
    bool m_activateKRunnerWhenTypingOnDesktop;

    // Accent color config
    Plasma::Containment *m_containment = nullptr;
    int m_containmentScreenId = -1;

    bool m_enteredEditMode;

#if PROJECT_VERSION_PATCH >= 80 || PROJECT_VERSION_MINOR >= 80
    bool m_showPreviewBanner;
#endif
};
