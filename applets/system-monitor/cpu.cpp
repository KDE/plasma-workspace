/*
 *   Copyright (C) 2008 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "cpu.h"
#include <QDebug>
#include <Plasma/Theme>
#include <KConfigDialog>
#include <QTimer>
#include <QGraphicsLinearLayout>
#include "plotter.h"

SM::Cpu::Cpu(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
    , m_rx("^cpu/(\\w+)/TotalLoad$")
{
    setHasConfigurationInterface(true);
    resize(234 + 20 + 23, 135 + 20 + 25);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sourceTimer.setSingleShot(true);
    connect(&m_sourceTimer, SIGNAL(timeout()), this, SLOT(sourcesChanged()));
}

SM::Cpu::~Cpu()
{
}

void SM::Cpu::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("CPU"));

    /* At the time this method is running, not all source may be connected. */
    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceChanged(QString)));
    connect(engine(), SIGNAL(sourceRemoved(QString)), this, SLOT(sourceChanged(QString)));
    foreach (const QString& source, engine()->sources()) {
        sourceChanged(source);
    }
}

void SM::Cpu::sourceChanged(const QString& name)
{
    if (m_rx.indexIn(name) != -1) {
        //qDebug() << m_rx.cap(1);
        //kWarning() << name; // debug
        m_cpus << name;
        if (!m_sourceTimer.isActive()) {
            m_sourceTimer.start(0);
        }
    }
}

void SM::Cpu::sourcesChanged()
{
    configChanged();
}

void SM::Cpu::configChanged()
{
    KConfigGroup cg = config();
    QStringList default_cpus;

    if(m_cpus.contains("cpu/system/TotalLoad"))
        default_cpus << "cpu/system/TotalLoad";
    else
        default_cpus = m_cpus;
    setInterval(cg.readEntry("interval", 2.0) * 1000.0);
    setSources(cg.readEntry("cpus", default_cpus));
    connectToEngine();
}

QString SM::Cpu::cpuTitle(const QString &name)
{
    if (name == "system") {
        return i18n("total");
    }
    return name;
}

bool SM::Cpu::addVisualization(const QString& source)
{
    QStringList l = source.split('/');
    if (l.count() < 3) {
        return false;
    }
    QString cpu = l[1];
    SM::Plotter *plotter = new SM::Plotter(this);
    plotter->setMinMax(0.0, 100.0);
    plotter->setTitle(cpuTitle(cpu));
    plotter->setUnit("%");
    appendVisualization(source, plotter);
    setPreferredItemHeight(80);
    return true;
}

void SM::Cpu::dataUpdated(const QString& source, const Plasma::DataEngine::Data &data)
{
    SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(source));
    if (plotter) {
        double value = data["value"].toDouble();
        QString temp = KGlobal::locale()->formatNumber(value, 1);
        plotter->addSample(QList<double>() << value);
        if (mode() == SM::Applet::Panel) {
            setToolTip(source, QString("<tr><td>%1&nbsp;</td><td>%2%</td></tr>")
                                      .arg(plotter->title()).arg(temp));
        }
    }
}

void SM::Cpu::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_model.clear();
    m_model.setHorizontalHeaderLabels(QStringList() << i18n("CPU"));
    QStandardItem *parentItem = m_model.invisibleRootItem();

    foreach (const QString& cpu, m_cpus) {
        if (m_rx.indexIn(cpu) != -1) {
            QStandardItem *item1 = new QStandardItem(cpuTitle(m_rx.cap(1)));
            item1->setEditable(false);
            item1->setCheckable(true);
            item1->setData(cpu);
            if (sources().contains(cpu)) {
                item1->setCheckState(Qt::Checked);
            }
            parentItem->appendRow(QList<QStandardItem *>() << item1);
        }
    }
    ui.treeView->setModel(&m_model);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->addPage(widget, i18n("CPUs"), "cpu");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void SM::Cpu::configAccepted()
{
    KConfigGroup cg = config();
    QStandardItem *parentItem = m_model.invisibleRootItem();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            if (item->checkState() == Qt::Checked) {
                appendSource(item->data().toString());
            }
        }
    }
    cg.writeEntry("cpus", sources());

    double interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

#include "cpu.moc"
