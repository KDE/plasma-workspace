/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <PlasmaQuick/ConfigView>
#include <PlasmaQuick/ContainmentView>
#include <QPointer>

#include <KConfigWatcher>

namespace KWayland
{
namespace Client
{
class PlasmaShellSurface;
}
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

    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)

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

private:
    void coronaPackageChanged(const KPackage::Package &package);
    void ensureWindowType();
    void setupWaylandIntegration();
    void setAccentColorFromWallpaper(const QColor &accentColor);
    bool handleKRunnerTextInput(QKeyEvent *e);

    QColor m_accentColor;
    QPointer<PlasmaQuick::ConfigView> m_configView;
    QPointer<QScreen> m_screenToFollow;
    KWayland::Client::PlasmaShellSurface *m_shellSurface;
    QString m_krunnerText;

    // KRunner config
    KConfigWatcher::Ptr m_configWatcher;
    bool m_activateKRunnerWhenTypingOnDesktop;

    // Accent color config
    Plasma::Containment *m_containment = nullptr;
    int m_containmentScreenId = -1;
};
