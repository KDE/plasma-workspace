/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "currentcontainmentactionsmodel.h"

#include <QMouseEvent>

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVBoxLayout>
#include <QWindow>

#include <KAboutPluginDialog>
#include <klocalizedstring.h>

#include <Plasma/Containment>
#include <Plasma/ContainmentActions>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

CurrentContainmentActionsModel::CurrentContainmentActionsModel(Plasma::Containment *containment, QObject *parent)
    : QStandardItemModel(parent)
    , m_containment(containment)
    , m_tempConfigParent(QString(), KConfig::SimpleConfig)
{
    m_baseCfg = KConfigGroup(m_containment->corona()->config(), "ActionPlugins");
    m_baseCfg = KConfigGroup(&m_baseCfg, QString::number(m_containment->containmentType()));

    QHash<QString, Plasma::ContainmentActions *> actions = containment->containmentActions();

    QHashIterator<QString, Plasma::ContainmentActions *> i(actions);
    while (i.hasNext()) {
        i.next();

        QStandardItem *item = new QStandardItem();
        item->setData(i.key(), ActionRole);
        item->setData(i.value()->metadata().pluginId(), PluginNameRole);

        m_plugins[i.key()] = Plasma::PluginLoader::self()->loadContainmentActions(m_containment, i.value()->metadata().pluginId());
        m_plugins[i.key()]->setContainment(m_containment);
        KConfigGroup cfg(&m_baseCfg, i.key());
        m_plugins[i.key()]->restore(cfg);
        item->setData(m_plugins[i.key()]->metadata().rawData().value(QStringLiteral("X-Plasma-HasConfigurationInterface")).toBool(),
                      HasConfigurationInterfaceRole);

        appendRow(item);
    }
}

CurrentContainmentActionsModel::~CurrentContainmentActionsModel()
{
}

QHash<int, QByteArray> CurrentContainmentActionsModel::roleNames() const
{
    return {
        {ActionRole, "action"},
        {PluginNameRole, "pluginName"},
        {HasConfigurationInterfaceRole, "hasConfigurationInterface"},
    };
}

QString CurrentContainmentActionsModel::mouseEventString(int mouseButton, int modifiers)
{
    QMouseEvent *mouse =
        new QMouseEvent(QEvent::MouseButtonRelease, QPoint(), (Qt::MouseButton)mouseButton, (Qt::MouseButton)mouseButton, (Qt::KeyboardModifiers)modifiers);

    const QString string = Plasma::ContainmentActions::eventToString(mouse);

    delete mouse;

    return string;
}

QString CurrentContainmentActionsModel::wheelEventString(const QPointF &delta, int mouseButtons, int modifiers)
{
    QWheelEvent *wheel = new QWheelEvent(QPointF(),
                                         QPointF(),
                                         delta.toPoint(),
                                         QPoint(),
                                         0,
                                         qAbs(delta.x()) > qAbs(delta.y()) ? Qt::Horizontal : Qt::Vertical,
                                         (Qt::MouseButtons)mouseButtons,
                                         (Qt::KeyboardModifiers)modifiers);

    const QString string = Plasma::ContainmentActions::eventToString(wheel);

    delete wheel;

    return string;
}

bool CurrentContainmentActionsModel::isTriggerUsed(const QString &trigger)
{
    return m_plugins.contains(trigger);
}

bool CurrentContainmentActionsModel::append(const QString &action, const QString &plugin)
{
    if (m_plugins.contains(action)) {
        return false;
    }

    QStandardItem *item = new QStandardItem();
    item->setData(action, ActionRole);
    item->setData(plugin, PluginNameRole);

    Plasma::ContainmentActions *actions = Plasma::PluginLoader::self()->loadContainmentActions(m_containment, plugin);

    if (!actions) {
        return false;
    }

    m_plugins[action] = actions;
    m_plugins[action]->setContainment(m_containment);
    // empty config: the new one will ne in default state
    KConfigGroup tempConfig(&m_tempConfigParent, "test");
    m_plugins[action]->restore(tempConfig);
    item->setData(m_plugins[action]->metadata().rawData().value(QStringLiteral("X-Plasma-HasConfigurationInterface")).toBool(), HasConfigurationInterfaceRole);
    m_removedTriggers.removeAll(action);

    appendRow(item);

    emit configurationChanged();
    return true;
}

void CurrentContainmentActionsModel::update(int row, const QString &action, const QString &plugin)
{
    const QString oldPlugin = itemData(index(row, 0)).value(PluginNameRole).toString();
    const QString oldTrigger = itemData(index(row, 0)).value(ActionRole).toString();

    if (oldTrigger == action && oldPlugin == plugin) {
        return;
    }

    QModelIndex idx = index(row, 0);

    if (idx.isValid()) {
        setData(idx, action, ActionRole);
        setData(idx, plugin, PluginNameRole);

        delete m_plugins[oldTrigger];
        m_plugins.remove(oldTrigger);

        if (oldPlugin != plugin) {
            m_removedTriggers << oldTrigger;
        }

        if (!m_plugins.contains(action) || oldPlugin != plugin) {
            delete m_plugins[action];
            m_plugins[action] = Plasma::PluginLoader::self()->loadContainmentActions(m_containment, plugin);
            m_plugins[action]->setContainment(m_containment);
            // empty config: the new one will ne in default state
            KConfigGroup tempConfig(&m_tempConfigParent, "test");
            m_plugins[action]->restore(tempConfig);
            setData(idx,
                    m_plugins[action]->metadata().rawData().value(QStringLiteral("X-Plasma-HasConfigurationInterface")).toBool(),
                    HasConfigurationInterfaceRole);
        }

        emit configurationChanged();
    }
}

void CurrentContainmentActionsModel::remove(int row)
{
    const QString action = itemData(index(row, 0)).value(ActionRole).toString();
    removeRows(row, 1);

    if (m_plugins.contains(action)) {
        delete m_plugins[action];
        m_plugins.remove(action);
        m_removedTriggers << action;
        emit configurationChanged();
    }
}

void CurrentContainmentActionsModel::showConfiguration(int row, QQuickItem *ctx)
{
    const QString action = itemData(index(row, 0)).value(ActionRole).toString();

    if (!m_plugins.contains(action)) {
        return;
    }

    QDialog *configDlg = new QDialog();
    configDlg->setAttribute(Qt::WA_DeleteOnClose);
    QLayout *lay = new QVBoxLayout(configDlg);
    configDlg->setLayout(lay);
    if (ctx && ctx->window()) {
        configDlg->setWindowModality(Qt::WindowModal);
        configDlg->winId(); // so it creates the windowHandle();
        configDlg->windowHandle()->setTransientParent(ctx->window());
    }

    Plasma::ContainmentActions *pluginInstance = m_plugins[action];
    // put the config in the dialog
    QWidget *w = pluginInstance->createConfigurationInterface(configDlg);
    QString title;
    if (w) {
        lay->addWidget(w);
        title = w->windowTitle();
    }

    configDlg->setWindowTitle(title.isEmpty() ? i18n("Configure Mouse Actions Plugin") : title);
    // put buttons below
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, configDlg);
    lay->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, configDlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, configDlg, &QDialog::reject);

    QObject::connect(configDlg, &QDialog::accepted, pluginInstance, [pluginInstance]() {
        pluginInstance->configurationAccepted();
    });

    connect(configDlg, &QDialog::accepted, this, &CurrentContainmentActionsModel::configurationChanged);

    connect(pluginInstance, &QObject::destroyed, configDlg, &QDialog::reject);

    configDlg->show();
}

void CurrentContainmentActionsModel::showAbout(int row, QQuickItem *ctx)
{
    const QString action = itemData(index(row, 0)).value(ActionRole).toString();

    if (!m_plugins.contains(action)) {
        return;
    }

    KPluginMetaData info = m_plugins[action]->metadata();

    auto aboutDialog = new KAboutPluginDialog(info, qobject_cast<QWidget *>(parent()));
    aboutDialog->setWindowIcon(QIcon::fromTheme(info.iconName()));
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);

    if (ctx && ctx->window()) {
        aboutDialog->setWindowModality(Qt::WindowModal);
        aboutDialog->winId(); // so it creates the windowHandle();
        aboutDialog->windowHandle()->setTransientParent(ctx->window());
    }
    aboutDialog->show();
}

void CurrentContainmentActionsModel::save()
{
    foreach (const QString &removedTrigger, m_removedTriggers) {
        m_containment->setContainmentActions(removedTrigger, QString());
    }
    m_removedTriggers.clear();

    QHashIterator<QString, Plasma::ContainmentActions *> i(m_plugins);
    while (i.hasNext()) {
        i.next();

        KConfigGroup cfg(&m_baseCfg, i.key());
        i.value()->save(cfg);

        m_containment->setContainmentActions(i.key(), i.value()->metadata().pluginId());
    }
}

#include "moc_currentcontainmentactionsmodel.cpp"
