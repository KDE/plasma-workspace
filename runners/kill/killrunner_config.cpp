/* Copyright 2009  <Jan Gerrit Marker> <jangerrit@weiler-marker.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

//Project-Includes
#include "killrunner_config.h"
//KDE-Includes
#include <KSharedConfig>
#include <KPluginFactory>
#include <krunner/abstractrunner.h>

K_PLUGIN_FACTORY(KillRunnerConfigFactory, registerPlugin<KillRunnerConfig>("kcm_krunner_kill");)

KillRunnerConfigForm::KillRunnerConfigForm(QWidget* parent) : QWidget(parent)
{
    setupUi(this);
}

KillRunnerConfig::KillRunnerConfig(QWidget* parent, const QVariantList& args) :
        KCModule(parent, args)
{
    m_ui = new KillRunnerConfigForm(this);

    QGridLayout* layout = new QGridLayout(this);
    layout->addWidget(m_ui, 0, 0);

    m_ui->sorting->addItem(i18n("CPU usage"), CPU);
    m_ui->sorting->addItem(i18n("inverted CPU usage"), CPUI);
    m_ui->sorting->addItem(i18n("nothing"), NONE);

    connect(m_ui->useTriggerWord,SIGNAL(stateChanged(int)),this,SLOT(changed()));
    connect(m_ui->triggerWord,SIGNAL(textChanged(QString)),this,SLOT(changed()));
    connect(m_ui->sorting,SIGNAL(currentIndexChanged(int)),this,SLOT(changed()));

    load();
}

void KillRunnerConfig::load()
{
    KCModule::load();

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("krunnerrc"));
    KConfigGroup grp = cfg->group("Runners");
    grp = KConfigGroup(&grp, "Kill Runner");

    m_ui->useTriggerWord->setChecked(grp.readEntry(CONFIG_USE_TRIGGERWORD,true));
    m_ui->triggerWord->setText(grp.readEntry(CONFIG_TRIGGERWORD,i18n("kill")));
    m_ui->sorting->setCurrentIndex(m_ui->sorting->findData(grp.readEntry<int>(CONFIG_SORTING, (int) NONE)));

    emit changed(false);
}

void KillRunnerConfig::save()
{
    KCModule::save();

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("krunnerrc"));
    KConfigGroup grp = cfg->group("Runners");
    grp = KConfigGroup(&grp, "Kill Runner");

    grp.writeEntry(CONFIG_USE_TRIGGERWORD,m_ui->useTriggerWord->isChecked());
    grp.writeEntry(CONFIG_TRIGGERWORD,m_ui->triggerWord->text());
    grp.writeEntry(CONFIG_SORTING,m_ui->sorting->itemData(m_ui->sorting->currentIndex()));

    emit changed(false);
}

void KillRunnerConfig::defaults()
{
    KCModule::defaults();

    m_ui->useTriggerWord->setChecked(true);
    m_ui->triggerWord->setText(i18n("kill"));
    m_ui->sorting->setCurrentIndex(m_ui->sorting->findData((int) NONE));

    emit changed(true);
}

#include "killrunner_config.moc"