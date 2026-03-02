/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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

#include <klocalizedstring.h>

#include <Plasma/Containment>
#include <Plasma/ContainmentActions>
#include <Plasma/Corona>
#include <Plasma/PluginLoader>

using namespace Qt::StringLiterals;

CurrentContainmentActionsModel::CurrentContainmentActionsModel(Plasma::Containment *containment, QObject *parent)
    : QStandardItemModel(parent)
    , m_containment(containment)
    , m_tempConfigParent(QString(), KConfig::SimpleConfig)
{
    m_baseCfg = KConfigGroup(m_containment->corona()->config(), u"ActionPlugins"_s);
    m_baseCfg = KConfigGroup(&m_baseCfg, QString::number((int)m_containment->containmentType()));

    const auto actions = containment->containmentActionsList();
    for (const QString &actionName : actions) {
        auto action = containment->containmentActions(actionName);

        auto *item = new QStandardItem();
        item->setData(actionName, ActionRole);
        item->setData(action->id(), PluginNameRole);

        m_plugins[actionName] = std::unique_ptr<Plasma::ContainmentActions>(Plasma::PluginLoader::self()->loadContainmentActions(m_containment, action->id()));
        m_plugins[actionName]->setContainment(m_containment);
        KConfigGroup cfg(&m_baseCfg, actionName);
        m_plugins[actionName]->restore(cfg);
        item->setData(m_plugins[actionName]->hasConfigurationInterface(), HasConfigurationInterfaceRole);

        appendRow(item);
    }
}

CurrentContainmentActionsModel::~CurrentContainmentActionsModel() = default;

QHash<int, QByteArray> CurrentContainmentActionsModel::roleNames() const
{
    return {
        {ActionRole, QByteArrayLiteral("action")},
        {PluginNameRole, QByteArrayLiteral("pluginName")},
        {HasConfigurationInterfaceRole, QByteArrayLiteral("hasConfigurationInterface")},
    };
}

QString CurrentContainmentActionsModel::mouseEventString(int mouseButton, int modifiers)
{
    auto *mouse =
        new QMouseEvent(QEvent::MouseButtonRelease, QPoint(), (Qt::MouseButton)mouseButton, (Qt::MouseButton)mouseButton, (Qt::KeyboardModifiers)modifiers);

    const QString string = Plasma::ContainmentActions::eventToString(mouse);

    delete mouse;

    return string;
}

QString CurrentContainmentActionsModel::wheelEventString(QObject *quickWheelEvent)
{
    const QPoint angleDelta = quickWheelEvent->property("angleDelta").toPoint();
    const auto buttons = Qt::MouseButtons(quickWheelEvent->property("buttons").toInt());
    const auto modifiers = Qt::KeyboardModifiers(quickWheelEvent->property("modifiers").toInt());

    QWheelEvent wheel(QPointF(), QPointF(), QPoint(), angleDelta, buttons, modifiers, Qt::NoScrollPhase, false);

    return Plasma::ContainmentActions::eventToString(&wheel);
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

    auto *item = new QStandardItem();
    item->setData(action, ActionRole);
    item->setData(plugin, PluginNameRole);

    Plasma::ContainmentActions *actions = Plasma::PluginLoader::self()->loadContainmentActions(m_containment, plugin);

    if (!actions) {
        return false;
    }

    m_plugins[action] = std::unique_ptr<Plasma::ContainmentActions>(actions);
    m_plugins[action]->setContainment(m_containment);
    // empty config: the new one will ne in default state
    KConfigGroup tempConfig(&m_tempConfigParent, u"test"_s);
    m_plugins[action]->restore(tempConfig);
    item->setData(m_plugins[action]->hasConfigurationInterface(), HasConfigurationInterfaceRole);
    m_removedTriggers.removeAll(action);

    appendRow(item);

    Q_EMIT configurationChanged();
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

        m_plugins.erase(oldTrigger);

        if (oldPlugin != plugin) {
            m_removedTriggers << oldTrigger;
        }

        if (!m_plugins.contains(action) || oldPlugin != plugin) {
            m_plugins[action] = std::unique_ptr<Plasma::ContainmentActions>(Plasma::PluginLoader::self()->loadContainmentActions(m_containment, plugin));
            m_plugins[action]->setContainment(m_containment);
            // empty config: the new one will ne in default state
            KConfigGroup tempConfig(&m_tempConfigParent, u"test"_s);
            m_plugins[action]->restore(tempConfig);
            setData(idx, m_plugins[action]->hasConfigurationInterface(), HasConfigurationInterfaceRole);
        }

        Q_EMIT configurationChanged();
    }
}

void CurrentContainmentActionsModel::remove(int row)
{
    const QString action = itemData(index(row, 0)).value(ActionRole).toString();
    removeRows(row, 1);

    if (m_plugins.contains(action)) {
        m_plugins.erase(action);
        m_removedTriggers << action;
        Q_EMIT configurationChanged();
    }
}

void CurrentContainmentActionsModel::showConfiguration(int row, QQuickItem *ctx)
{
    const QString action = itemData(index(row, 0)).value(ActionRole).toString();

    if (!m_plugins.contains(action)) {
        return;
    }

    auto *configDlg = new QDialog();
    configDlg->setAttribute(Qt::WA_DeleteOnClose);
    QLayout *lay = new QVBoxLayout(configDlg);
    configDlg->setLayout(lay);
    if (ctx && ctx->window()) {
        configDlg->setWindowModality(Qt::WindowModal);
        configDlg->winId(); // so it creates the windowHandle();
        configDlg->windowHandle()->setTransientParent(ctx->window());
    }

    auto &pluginInstance = m_plugins[action];
    // put the config in the dialog
    QWidget *w = pluginInstance->createConfigurationInterface(configDlg);
    QString title;
    if (w) {
        lay->addWidget(w);
        title = w->windowTitle();
    }

    configDlg->setWindowTitle(title.isEmpty() ? i18n("Configure Mouse Actions Plugin") : title);
    // put buttons below
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, configDlg);
    lay->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, configDlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, configDlg, &QDialog::reject);

    QObject::connect(configDlg, &QDialog::accepted, pluginInstance.get(), [&pluginInstance]() {
        pluginInstance->configurationAccepted();
    });

    connect(configDlg, &QDialog::accepted, this, &CurrentContainmentActionsModel::configurationChanged);

    connect(pluginInstance.get(), &QObject::destroyed, configDlg, &QDialog::reject);

    configDlg->show();
}

void CurrentContainmentActionsModel::save()
{
    for (const QString &removedTrigger : std::as_const(m_removedTriggers)) {
        m_containment->setContainmentActions(removedTrigger, QString());
    }
    m_removedTriggers.clear();

    for (auto &[key, value] : m_plugins) {
        KConfigGroup cfg(&m_baseCfg, key);
        value->save(cfg);

        m_containment->setContainmentActions(key, value->id());
    }
}

#include "moc_currentcontainmentactionsmodel.cpp"
