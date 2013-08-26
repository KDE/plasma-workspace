/*
 *   Copyright (c) 2009 Chani Armitage <chani@kde.org>
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

#include "mousepluginwidget.h"

#include <Plasma/Containment>

#include <KAboutData>
#include <KAboutApplicationDialog>
#include <KDebug>

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>

Q_DECLARE_METATYPE(KPluginInfo)

MousePluginWidget::MousePluginWidget(const QString &pluginName, const QString &trigger, QGridLayout *layoutHack, QWidget *parent)
    : QObject(parent),
      m_configDlg(0),
      m_containment(0),
      m_lastConfigLocation(trigger),
      m_tempConfigParent(QString(), KConfig::SimpleConfig)
{
    KPluginInfo::List plugins = Plasma::ContainmentActions::listContainmentActionsInfo();
    if (plugins.isEmpty()) {
        //panic!!
        QLabel *fail = new QLabel(i18n("No plugins found, check your installation."), parent);
        layoutHack->addWidget(fail, 0, 0);
        return;
    }

    //make us some widgets
    m_pluginList = new QComboBox(parent);
    m_aboutButton = new QToolButton(parent);
    m_clearButton = new QToolButton(parent);
    m_triggerButton = new MouseInputButton(parent);
    m_configButton = new QToolButton(parent);
    //m_ui.description->setText(plugin.comment());

    //plugin list
    //FIXME is there some way to share this across all the entries?
    foreach (const KPluginInfo& plugin, plugins) {
        if (plugin.property("NoDisplay").toBool()) {
            continue;
        }

        m_pluginList->addItem(KIcon(plugin.icon()), plugin.name(), QVariant::fromValue(plugin));
        if (plugin.pluginName() == pluginName) {
            m_pluginList->setCurrentIndex(m_pluginList->count() - 1);
            m_plugin = plugin;
        }
    }

    if (! m_plugin.isValid()) {
        //probably an empty string; pick the first one
        m_pluginList->setCurrentIndex(0);
        m_plugin = plugins.first();
    }

    //I can haz config?
    m_tempConfig = KConfigGroup(&m_tempConfigParent, "test");
    if (!m_plugin.property("X-Plasma-HasConfigurationInterface").toBool()) {
        m_configButton->setVisible(false);
    }

    setTrigger(trigger);

    //pretty icons for the buttons
    m_aboutButton->setIcon(KIcon("dialog-information"));
    m_aboutButton->setToolTip(i18nc("About mouse action", "About"));
    m_triggerButton->setIcon(KIcon("input-mouse"));
    m_configButton->setIcon(KIcon("configure"));
    m_configButton->setToolTip(i18nc("Configure mouse action", "Configure"));
    m_clearButton->setIcon(KIcon("list-remove"));
    m_clearButton->setToolTip(i18nc("Remove mouse action", "Remove"));

    //HACK
    //FIXME what's the Right Way to do this?
    int row = layoutHack->rowCount();
    layoutHack->addWidget(m_triggerButton, row, 0);
    layoutHack->addWidget(m_pluginList, row, 1);
    layoutHack->addWidget(m_configButton, row, 2);
    layoutHack->addWidget(m_aboutButton, row, 3);
    layoutHack->addWidget(m_clearButton, row, 4);

    //connect
    connect(m_pluginList, SIGNAL(currentIndexChanged(int)), this, SLOT(setPlugin(int)));
    connect(m_triggerButton, SIGNAL(triggerChanged(QString,QString)), this, SLOT(changeTrigger(QString,QString)));
    connect(m_configButton, SIGNAL(clicked()), this, SLOT(configure()));
    connect(m_clearButton, SIGNAL(clicked()), this, SLOT(clearTrigger()));
    connect(m_aboutButton, SIGNAL(clicked()), this, SLOT(showAbout()));
}

MousePluginWidget::~MousePluginWidget()
{
    delete m_pluginInstance.data();
}

void MousePluginWidget::setPlugin(int index)
{
    m_plugin = m_pluginList->itemData(index).value<KPluginInfo>();
    //clear all the old plugin's stuff
    if (m_configDlg) {
        kDebug() << "can't happen";
        m_configDlg->deleteLater();
        m_configDlg = 0;
    }
    if (m_pluginInstance) {
        delete m_pluginInstance.data();
        kDebug() << "tried to delete instance";
    }
    //reset config button
    bool hasConfig = m_plugin.property("X-Plasma-HasConfigurationInterface").toBool();
    m_configButton->setVisible(hasConfig);
    m_tempConfig.deleteGroup();
    m_lastConfigLocation.clear();
    //FIXME a stray mousewheel deleting your config would be no fun.
    //perhaps we could mark it for deletion, and then... um.. not delete it if the user goes back to
    //that plugin? but then we'd have to record which plugin the config is *for*...
    emit configChanged(m_triggerButton->trigger());
}

void MousePluginWidget::setContainment(Plasma::Containment *ctmt)
{
    //note: since the old plugin's parent is the old containment,
    //we let that containment take care of deleting it
    m_containment = ctmt;
}

void MousePluginWidget::setTrigger(const QString &trigger)
{
    m_triggerButton->setTrigger(trigger);
    updateConfig(trigger);
}

void MousePluginWidget::clearTrigger()
{
    QString oldTrigger = m_triggerButton->trigger();
    setTrigger(QString());
    emit triggerChanged(oldTrigger, QString());

    //byebye!
    m_pluginList->deleteLater();
    m_aboutButton->deleteLater();
    m_clearButton->deleteLater();
    m_triggerButton->deleteLater();
    m_configButton->deleteLater();

    deleteLater();
}

void MousePluginWidget::changeTrigger(const QString &oldTrigger, const QString& newTrigger)
{
    updateConfig(newTrigger);
    emit triggerChanged(oldTrigger, newTrigger);
}

void MousePluginWidget::updateConfig(const QString &trigger)
{
    m_configButton->setEnabled(!trigger.isEmpty());
    m_pluginList->setEnabled(!trigger.isEmpty());
}

void MousePluginWidget::configure()
{
    if (!m_pluginInstance) {
        Plasma::ContainmentActions *pluginInstance = Plasma::ContainmentActions::load(m_containment, m_plugin.pluginName());
        if (!pluginInstance) {
            //FIXME tell user
            kDebug() << "failed to load plugin!";
            return;
        }

        m_pluginInstance = pluginInstance;

        if (m_lastConfigLocation.isEmpty()) {
            pluginInstance->restore(m_tempConfig);
        } else {
            KConfigGroup cfg = m_containment->containmentActionsConfig();
            cfg = KConfigGroup(&cfg, m_lastConfigLocation);
            pluginInstance->restore(cfg);
        }
    }

    if (!m_configDlg) {
        m_configDlg = new QDialog(qobject_cast<QWidget*>(parent()));
        QLayout *lay = new QVBoxLayout(m_configDlg);
        m_configDlg->setLayout(lay);
        m_configDlg->setWindowModality(Qt::WindowModal);

        //put the config in the dialog
        QWidget *w = m_pluginInstance.data()->createConfigurationInterface(m_configDlg);
        if (w) {
            lay->addWidget(w);
        }
	const QString title = w->windowTitle();

        m_configDlg->setWindowTitle(title.isEmpty() ? i18n("Configure Plugin") :title);
        //put buttons below
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                         Qt::Horizontal, m_configDlg);
        lay->addWidget(buttons);

        //TODO other signals?
        connect(buttons, SIGNAL(accepted()), this, SLOT(acceptConfig()));
        connect(buttons, SIGNAL(rejected()), this, SLOT(rejectConfig()));
    }

    m_configDlg->show();
}

void MousePluginWidget::acceptConfig()
{
    kDebug() << "accept";
    if (m_pluginInstance) {
        m_pluginInstance.data()->configurationAccepted();
    }

    m_configDlg->deleteLater();
    m_configDlg = 0;
    emit configChanged(m_triggerButton->trigger());
}

void MousePluginWidget::rejectConfig()
{
    kDebug() << "reject";
    m_configDlg->deleteLater();
    m_configDlg = 0;
}

void MousePluginWidget::prepareForSave()
{
    if (!m_configButton->isVisible() || m_pluginInstance || m_lastConfigLocation.isEmpty()) {
        return;
    }

    //back up our config because it'll be erased for saving
    KConfigGroup cfg = m_containment->containmentActionsConfig();
    cfg = KConfigGroup(&cfg, m_lastConfigLocation);
    cfg.copyTo(&m_tempConfig);
    //kDebug() << "copied to temp";
}

void MousePluginWidget::save()
{
    const QString trigger = m_triggerButton->trigger();
    if (trigger.isEmpty()) {
        m_lastConfigLocation.clear();
        return;
    }

    if (m_pluginInstance || !m_lastConfigLocation.isEmpty()) {
        KConfigGroup cfg = m_containment->containmentActionsConfig();
        cfg = KConfigGroup(&cfg, trigger);
        if (m_pluginInstance) {
            m_pluginInstance.data()->save(cfg);
        } else {
            m_tempConfig.copyTo(&cfg);
        }
        m_lastConfigLocation = trigger;
    }
    m_containment->setContainmentActions(trigger, m_plugin.pluginName());
}

//copied from appletbrowser.cpp
//FIXME add a feature to KAboutApplicationDialog to delete the object
/* This is just a wrapper around KAboutApplicationDialog that deletes
the KAboutData object that it is associated with, when it is deleted.
This is required to free memory correctly when KAboutApplicationDialog
is called with a temporary KAboutData that is allocated on the heap.
(see the code below, in AppletBrowserWidget::infoAboutApplet())
*/
class KAboutApplicationDialog2 : public KAboutApplicationDialog
{
public:
    KAboutApplicationDialog2(KAboutData *ab, QWidget *parent = 0)
    : KAboutApplicationDialog(ab, parent), m_ab(ab) {}

    ~KAboutApplicationDialog2()
    {
        delete m_ab;
    }

private:
    KAboutData *m_ab;
};

void MousePluginWidget::showAbout()
{
    KAboutData *aboutData = new KAboutData(m_plugin.name().toUtf8(),
            m_plugin.name().toUtf8(),
            ki18n(m_plugin.name().toUtf8()),
            m_plugin.version().toUtf8(), ki18n(m_plugin.comment().toUtf8()),
            m_plugin.fullLicense().key(), ki18n(QByteArray()), ki18n(QByteArray()), m_plugin.website().toLatin1(),
            m_plugin.email().toLatin1());

    aboutData->setProgramIconName(m_plugin.icon());

    aboutData->addAuthor(ki18n(m_plugin.author().toUtf8()), ki18n(QByteArray()), m_plugin.email().toLatin1());

    KAboutApplicationDialog *aboutDialog = new KAboutApplicationDialog2(aboutData, qobject_cast<QWidget*>(parent()));
    aboutDialog->show();
}

// vim: sw=4 sts=4 et tw=100
