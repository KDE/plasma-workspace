/*
    SPDX-FileCopyrightText: 2009 Jan Gerrit Marker <jangerrit@weiler-marker.com>
    SPDX-FileCopyrightText: 2020-2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "killrunner_config.h"

#include "config_keys.h"
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

K_PLUGIN_FACTORY(KillRunnerConfigFactory, registerPlugin<KillRunnerConfig>();)

KillRunnerConfig::KillRunnerConfig(QObject *parent)
    : KCModule(parent)
{
    m_ui = new Ui::KillRunnerConfigUi();
    m_ui->setupUi(widget());

    m_ui->sorting->addItem(i18n("CPU usage"), CPU);
    m_ui->sorting->addItem(i18n("inverted CPU usage"), CPUI);
    m_ui->sorting->addItem(i18n("nothing"), NONE);

    connect(m_ui->useTriggerWord, &QCheckBox::stateChanged, this, &KillRunnerConfig::markAsChanged);
    connect(m_ui->triggerWord, &KLineEdit::textChanged, this, &KillRunnerConfig::markAsChanged);
    connect(m_ui->sorting, &QComboBox::currentIndexChanged, this, &KillRunnerConfig::markAsChanged);
}

void KillRunnerConfig::load()
{
    KCModule::load();

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("krunnerrc"));
    const KConfigGroup grp = cfg->group("Runners").group(KRUNNER_PLUGIN_NAME);

    m_ui->useTriggerWord->setChecked(grp.readEntry(CONFIG_USE_TRIGGERWORD, true));
    m_ui->triggerWord->setText(grp.readEntry(CONFIG_TRIGGERWORD, i18n("kill")));
    m_ui->sorting->setCurrentIndex(m_ui->sorting->findData(grp.readEntry<int>(CONFIG_SORTING, NONE)));

    setNeedsSave(false);
}

void KillRunnerConfig::save()
{
    KCModule::save();

    KSharedConfig::Ptr cfg = KSharedConfig::openConfig(QStringLiteral("krunnerrc"));
    KConfigGroup grp = cfg->group("Runners").group(KRUNNER_PLUGIN_NAME);

    grp.writeEntry(CONFIG_USE_TRIGGERWORD, m_ui->useTriggerWord->isChecked());
    grp.writeEntry(CONFIG_TRIGGERWORD, m_ui->triggerWord->text());
    grp.writeEntry(CONFIG_SORTING, m_ui->sorting->itemData(m_ui->sorting->currentIndex()));
    grp.sync();

    setNeedsSave(false);
}

void KillRunnerConfig::defaults()
{
    KCModule::defaults();

    m_ui->useTriggerWord->setChecked(true);
    m_ui->triggerWord->setText(i18n("kill"));
    m_ui->sorting->setCurrentIndex(m_ui->sorting->findData((int)NONE));

    setNeedsSave(true);
}

#include "killrunner_config.moc"
