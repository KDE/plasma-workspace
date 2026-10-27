/*
    SPDX-FileCopyrightText: 2026 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include "plasmoidtestitem.h"

#include <QAccessible>
#include <QDebug>

#include <Plasma/Applet>
#include <PlasmaQuick/AppletQuickItem>

#include <functional>

#include "../plasmawindowedcorona.h"

// Private fallback for reading explicitly-declared Accessible.name/description
// when the public QAccessible interface is unavailable (no a11y platform plugin).
// Only compiled in when the private QtQuick headers are on the include path
// (link Qt6::QuickPrivate / add its include dir); otherwise we rely on the
// public QAccessible path below.
#if __has_include(<QtQuick/private/qquickaccessibleattached_p.h>)
#define HAVE_QQUICKACCESSIBLE_ATTACHED_PRIVATE 1
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

PlasmoidTestItem::PlasmoidTestItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

PlasmoidTestItem::~PlasmoidTestItem() = default;

void PlasmoidTestItem::componentComplete()
{
    QQuickItem::componentComplete();
    m_componentComplete = true;
    if (!m_appletId.isEmpty()) {
        loadApplet();
    }
}

void PlasmoidTestItem::setAppletId(const QString &id)
{
    if (m_appletId == id) {
        return;
    }
    m_appletId = id;
    Q_EMIT appletIdChanged();
    if (m_componentComplete) {
        loadApplet();
    }
}

void PlasmoidTestItem::setShellPackage(const QString &shell)
{
    if (m_shellPackage == shell) {
        return;
    }
    if (m_corona) {
        qWarning() << "PlasmoidTestItem: shellPackage must be set before appletId loads the corona";
        return;
    }
    m_shellPackage = shell;
    Q_EMIT shellPackageChanged();
}

void PlasmoidTestItem::loadApplet()
{
    if (m_appletId.isEmpty()) {
        return;
    }
    setReady(false);

    if (!m_corona) {
        // Reuses PlasmaWindowedCorona's load(): loads the shell package and
        // creates the "empty" containment, exactly like plasmawindowed.
        m_corona = std::make_unique<PlasmaWindowedCorona>(m_shellPackage);
    }

    Plasma::Applet *applet = m_corona->loadAppletWithoutView(m_appletId, {});
    if (!applet) {
        qWarning() << "PlasmoidTestItem: failed to load applet" << m_appletId;
        return;
    }

    auto *item = PlasmaQuick::AppletQuickItem::itemForApplet(applet);
    if (!item) {
        qWarning() << "PlasmoidTestItem: no AppletQuickItem for" << m_appletId;
        return;
    }

    m_appletInterface = item;
    item->setParentItem(this);
    item->setVisible(true);
    // Don't let the applet vanish when the test window loses focus.
    item->setProperty("hideOnWindowDeactivate", false);
    syncAppletSize();

    Q_EMIT appletInterfaceChanged();
    // The applet is loaded and hosted here; its QML content is instantiated
    // asynchronously afterwards (see isReady()).
    setReady(true);
}

void PlasmoidTestItem::syncAppletSize()
{
    if (m_appletInterface) {
        m_appletInterface->setSize(size());
    }
}

void PlasmoidTestItem::setReady(bool ready)
{
    if (m_ready == ready) {
        return;
    }
    m_ready = ready;
    Q_EMIT readyChanged();
}

void PlasmoidTestItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    syncAppletSize();
}

QString PlasmoidTestItem::accessibleName(QQuickItem *item)
{
    if (!item) {
        return {};
    }

    // Primary: public QAccessible API — this is what at-spi exposes, including
    // names derived from role/text content.
    if (auto *iface = QAccessible::queryAccessibleInterface(item)) {
        const QString text = iface->text(QAccessible::Name);
        if (!text.isEmpty()) {
            return text;
        }
        // at-spi derives the accessible name from the text content of StaticText
        // (QQuickText) and other text-bearing roles, but queryAccessibleInterface
        // ->text(Name) does not always expose it. Use the text interface content
        // so e.g. TabButton labels ("Days"/"Months"/"Years") are locatable.
        if (auto *ti = iface->textInterface()) {
            const QString content = ti->text(0, ti->characterCount());
            if (!content.isEmpty()) {
                return content;
            }
        }
    }

    // Last resort: a QML "text" property (QQuickText/Label/Button text).
    // Mirror at-spi text-derived names.
    const QVariant textProp = item->property("text");
    if (textProp.isValid() && textProp.canConvert<QString>()) {
        const QString s = textProp.toString();
        if (!s.isEmpty()) {
            return s;
        }
    }

    // Fallback: read the explicitly-declared Accessible attached property.
    // Only available when the private QtQuick header is present.
#ifdef HAVE_QQUICKACCESSIBLE_ATTACHED_PRIVATE
    if (auto *a = QQuickAccessibleAttached::attachedProperties(item)) {
        return a->name();
    }
#endif
    return {};
}

QString PlasmoidTestItem::accessibleDescription(QQuickItem *item)
{
    if (!item) {
        return {};
    }

    if (auto *iface = QAccessible::queryAccessibleInterface(item)) {
        const QString text = iface->text(QAccessible::Description);
        if (!text.isEmpty()) {
            return text;
        }
    }

#ifdef HAVE_QQUICKACCESSIBLE_ATTACHED_PRIVATE
    if (auto *a = QQuickAccessibleAttached::attachedProperties(item)) {
        return a->description();
    }
#endif
    return {};
}

QString PlasmoidTestItem::strategyValue(QQuickItem *item, PlasmoidTestBy::Strategy by)
{
    switch (by) {
    case PlasmoidTestBy::Strategy::NAME:
        return accessibleName(item);
    case PlasmoidTestBy::Strategy::DESCRIPTION:
        return accessibleDescription(item);
    case PlasmoidTestBy::Strategy::ACCESSIBILITY_ID:
        return item ? item->objectName() : QString();
    }
    return {};
}

void PlasmoidTestItem::collectMatches(QQuickItem *parent, PlasmoidTestBy::Strategy by, const QString &value, QList<QQuickItem *> &out)
{
    if (!parent) {
        return;
    }
    for (QQuickItem *child : parent->childItems()) {
        if (strategyValue(child, by) == value) {
            out.append(child);
        }
        collectMatches(child, by, value, out);
    }
}

QQuickItem *PlasmoidTestItem::findItem(PlasmoidTestBy::Strategy by, const QString &value) const
{
    const auto all = findItems(by, value);
    return all.isEmpty() ? nullptr : all.first();
}

QList<QQuickItem *> PlasmoidTestItem::findItems(PlasmoidTestBy::Strategy by, const QString &value) const
{
    QList<QQuickItem *> out;
    collectMatches(m_appletInterface, by, value, out);
    return out;
}

QString PlasmoidTestItem::dumpTree() const
{
    QString out;
    std::function<void(QQuickItem *, const QString &)> walk = [&](QQuickItem *node, const QString &p) {
        if (!node) {
            return;
        }
        const QString name = accessibleName(node);
        const QString desc = accessibleDescription(node);
        const QString cls = QString::fromLatin1(node->metaObject()->className());
        out += QStringLiteral("%1[%2] objectName=\"%3\" name=\"%4\" desc=\"%5\"\n").arg(p, cls, node->objectName(), name, desc);
        for (QQuickItem *child : node->childItems()) {
            walk(child, p + QLatin1String("  "));
        }
    };
    walk(m_appletInterface, QString());
    return out;
}
