/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QPointer>
#include <QQuickItem>

#include <memory>

#include "plasmoidtestby.h"

class PlasmaWindowedCorona;

namespace Plasma
{
class Applet;
}

/**
 * A QQuickItem that hosts a Plasma applet by plugin id, for use as the root of
 * a QML TestCase run via qmltestrunner. The real test window is qmltestrunner's
 * internal QQuickView; the applet's AppletQuickItem is parented into this item so
 * that QTest.mouseClick/keyClick (which route through item.window) deliver there.
 *
 * Applet loading reuses PlasmaWindowedCorona (via loadAppletWithoutView), so the
 * containment/shell/config setup is identical to plasmawindowed — only the
 * PlasmaWindowedView is skipped, because the applet is hosted in this item instead.
 *
 * Replaces appium/at-spi "find_element" locators with Accessible-property based
 * lookup (Accessible.name / Accessible.description), via QAccessible.
 *
 * Limitation: qmltestrunner is a QGuiApplication, so applets that construct
 * QWidgets while loading crash the process (digital clock aborts in QMenu, the
 * notifications applet and the system tray segfault in QWidget::insertAction)
 * instead of failing the test; those stay in the appium suite, which runs them
 * inside plasmawindowed (a QApplication).
 */
class PlasmoidTestItem : public QQuickItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlasmoidTestItem)

public:
    Q_PROPERTY(QString appletId READ appletId WRITE setAppletId NOTIFY appletIdChanged)
    Q_PROPERTY(QString shellPackage READ shellPackage WRITE setShellPackage NOTIFY shellPackageChanged)
    Q_PROPERTY(QQuickItem *appletInterface READ appletInterface NOTIFY appletInterfaceChanged)
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)

    explicit PlasmoidTestItem(QQuickItem *parent = nullptr);
    ~PlasmoidTestItem() override;

    QString appletId() const
    {
        return m_appletId;
    }
    void setAppletId(const QString &id);

    QString shellPackage() const
    {
        return m_shellPackage;
    }
    void setShellPackage(const QString &shell);

    QQuickItem *appletInterface() const
    {
        return m_appletInterface;
    }

    /**
     * True once the applet is loaded and its AppletQuickItem is hosted here.
     *
     * The applet is loaded together with this item, so tests normally see this
     * already true; they still assert it so a broken load (wrong appletId,
     * missing shell package) fails with a clear message, e.g. in init():
     *
     *     when: windowShown
     *     function init() {
     *         tryVerify(() => plasmoid.ready, 10000, "applet did not become ready")
     *     }
     *
     * This only means "loaded", not "content exists": applet content may be
     * created asynchronously afterwards, and there is no universal readiness
     * signal (applets may use compact/full representations or inline content),
     * so a test that needs more should wait for the specific item it looks for.
     */
    bool isReady() const
    {
        return m_ready;
    }

    /**
     * First descendant of the applet matching appium's find_element(By, value):
     * By.NAME matches Accessible.name, By.DESCRIPTION matches Accessible.description,
     * By.ACCESSIBILITY_ID matches objectName().
     */
    Q_INVOKABLE QQuickItem *findItem(PlasmoidTestBy::Strategy by, const QString &value) const;
    /** All matches (mirrors appium's find_elements). */
    Q_INVOKABLE QList<QQuickItem *> findItems(PlasmoidTestBy::Strategy by, const QString &value) const;

    /** Debug helper: dump name/description/objectName of every descendant. */
    Q_INVOKABLE QString dumpTree() const;

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

Q_SIGNALS:
    void appletIdChanged();
    void shellPackageChanged();
    void appletInterfaceChanged();
    void readyChanged();

private:
    void loadApplet();
    void syncAppletSize();
    void setReady(bool ready);
    static void collectMatches(QQuickItem *parent, PlasmoidTestBy::Strategy by, const QString &value, QList<QQuickItem *> &out);
    static QString strategyValue(QQuickItem *item, PlasmoidTestBy::Strategy by);
    static QString accessibleName(QQuickItem *item);
    static QString accessibleDescription(QQuickItem *item);

    QString m_appletId;
    QString m_shellPackage = QStringLiteral("org.kde.plasma.nano");
    QPointer<QQuickItem> m_appletInterface;
    std::unique_ptr<PlasmaWindowedCorona> m_corona;
    bool m_componentComplete = false;
    bool m_ready = false;
};
