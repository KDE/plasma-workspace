/*
 *   Copyright 2008 Ryan P. Bitanga <ryan.bitanga@gmail.com>
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

#include "configdialog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTimer>

#include <KConfigGroup>
#include <KDebug>
#include <KGlobal>
#include <KPluginInfo>
#include <KPluginSelector>
#include <KServiceTypeTrader>

#include <Plasma/RunnerManager>
#include <Plasma/Theme>

#include "interfaces/default/interface.h"
#include "krunnersettings.h"
#include "interfaces/quicksand/qs_dialog.h"

KRunnerConfigWidget::KRunnerConfigWidget(Plasma::RunnerManager *manager, QWidget *parent)
    : QWidget(parent),
      m_preview(0),
      m_manager(manager)
{
    m_tabWidget = new KTabWidget(this);
    m_sel = new KPluginSelector(m_tabWidget);
    m_tabWidget->addTab(m_sel, i18n("Plugins"));

    QWidget *m_generalSettings = new QWidget(m_tabWidget);
    //QVBoxLayout *genLayout = new QVBoxLayout(m_generalSettings);

    m_interfaceType = KRunnerSettings::interface();
    m_uiOptions.setupUi(m_generalSettings);

    syncPalette();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(syncPalette()));

    QButtonGroup *positionButtons = new QButtonGroup(m_generalSettings);
    positionButtons->addButton(m_uiOptions.topEdgeButton);
    positionButtons->addButton(m_uiOptions.freeFloatingButton);
    m_uiOptions.freeFloatingButton->setChecked(KRunnerSettings::freeFloating());

    QButtonGroup *displayButtons = new QButtonGroup(m_generalSettings);
    connect(displayButtons, SIGNAL(buttonClicked(int)), this, SLOT(setInterface(int)));
    displayButtons->addButton(m_uiOptions.commandButton, KRunnerSettings::EnumInterface::CommandOriented);
    displayButtons->addButton(m_uiOptions.taskButton, KRunnerSettings::EnumInterface::TaskOriented);

    if (m_interfaceType == KRunnerSettings::EnumInterface::CommandOriented) {
        m_uiOptions.commandButton->setChecked(true);
    } else {
        m_uiOptions.taskButton->setChecked(true);
    }

    connect(m_uiOptions.previewButton, SIGNAL(clicked()), this, SLOT(previewInterface()));

    m_tabWidget->addTab(m_generalSettings, i18n("User Interface"));

    connect(m_sel, SIGNAL(configCommitted(QByteArray)), this, SLOT(updateRunner(QByteArray)));

    QTimer::singleShot(0, this, SLOT(load()));

    m_buttons = new QDialogButtonBox(this);
    m_buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    connect(m_buttons, SIGNAL(clicked(QAbstractButton*)), this, SLOT(save(QAbstractButton*)));
    connect(m_buttons, SIGNAL(rejected()), this, SIGNAL(finished()));

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->addWidget(m_tabWidget);
    topLayout->addWidget(m_buttons);
}

KRunnerConfigWidget::~KRunnerConfigWidget()
{
}

void KRunnerConfigWidget::syncPalette()
{
    QColor color = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QPalette p = palette();
    p.setColor(QPalette::Normal, QPalette::WindowText, color);
    p.setColor(QPalette::Inactive, QPalette::WindowText, color);
    color.setAlphaF(0.6);
    p.setColor(QPalette::Disabled, QPalette::WindowText, color);

    p.setColor(QPalette::Normal, QPalette::Link, Plasma::Theme::defaultTheme()->color(Plasma::Theme::LinkColor));
    p.setColor(QPalette::Normal, QPalette::LinkVisited, Plasma::Theme::defaultTheme()->color(Plasma::Theme::VisitedLinkColor));
    setPalette(p);
}

void KRunnerConfigWidget::previewInterface()
{
    delete m_preview;
    switch (m_interfaceType) {
    case KRunnerSettings::EnumInterface::CommandOriented:
        m_preview = new Interface(m_manager, this);
        break;
    default:
        m_preview = new QsDialog(m_manager, this);
        break;
    }

    m_preview->setFreeFloating(m_uiOptions.freeFloatingButton->isChecked());
    m_preview->show();
}

void KRunnerConfigWidget::setInterface(int type)
{
    m_interfaceType = type;
}

void KRunnerConfigWidget::updateRunner(const QByteArray &name)
{
    Plasma::AbstractRunner *runner = m_manager->runner(QString::fromLatin1( name ));
    //Update runner if runner is loaded
    if (runner) {
        runner->reloadConfiguration();
    }
}

void KRunnerConfigWidget::load()
{
    m_sel->addPlugins(Plasma::RunnerManager::listRunnerInfo(),
                    KPluginSelector::ReadConfigFile,
                    i18n("Available Plugins"), QString(),
                    KSharedConfig::openConfig(QLatin1String( "krunnerrc" )));
}

void KRunnerConfigWidget::save(QAbstractButton *pushed)
{
    if (m_buttons->buttonRole(pushed) == QDialogButtonBox::ApplyRole ||
        m_buttons->buttonRole(pushed) == QDialogButtonBox::AcceptRole) {
        m_sel->save();
        m_manager->reloadConfiguration();
        KRunnerSettings::setInterface(m_interfaceType);
        KRunnerSettings::setFreeFloating(m_uiOptions.freeFloatingButton->isChecked());
        KRunnerSettings::self()->writeConfig();

        if (m_buttons->buttonRole(pushed) == QDialogButtonBox::AcceptRole) {
            emit finished();
        }
    }
}

#include "configdialog.moc"

