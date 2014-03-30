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

#include "net.h"
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>
#include <KConfigDialog>
#include <QGraphicsLinearLayout>
#include <plotter.h>

SM::Net::Net(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
    , m_rx("^network/interfaces/(\\w+)/transmitter/data$")
{
    setHasConfigurationInterface(true);
    resize(234 + 20 + 23, 135 + 20 + 25);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_sourceTimer.setSingleShot(true);
    connect(&m_sourceTimer, SIGNAL(timeout()), this, SLOT(sourcesAdded()));
}

SM::Net::~Net()
{
}

void SM::Net::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("Network"));

    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceAdded(QString)));
    connect(engine(), SIGNAL(sourceRemoved(QString)),
            this, SLOT(sourceRemoved(QString)));
    foreach (const QString& source, engine()->sources()) {
        sourceAdded(source);
    }
}

void SM::Net::configChanged()
{
    KConfigGroup cg = config();
    setInterval(cg.readEntry("interval", 2.0) * 1000);
    setSources(cg.readEntry("interfaces", m_interfaces));
    connectToEngine();
}

void SM::Net::sourceAdded(const QString& name)
{
    if (m_rx.indexIn(name) != -1) {
        //qDebug() << m_rx.cap(1);
        if (m_rx.cap(1) != "lo") {
            m_interfaces << name;
            if (!m_sourceTimer.isActive()) {
                m_sourceTimer.start(0);
            }
        }
    }
}

void SM::Net::sourcesAdded()
{
    configChanged();
}

void SM::Net::sourceRemoved(const QString& name)
{
    m_interfaces.removeAll(name);
}

bool SM::Net::addVisualization(const QString& source)
{
    QStringList l = source.split('/');
    if (l.count() < 3) {
        return false;
    }
    QString interface = l[2];
    SM::Plotter *plotter = new SM::Plotter(this);
    plotter->setTitle(interface);
    plotter->setUnit("KiB/s");
    plotter->setCustomPlots(QList<QColor>() << QColor("#0099ff") << QColor("#91ff00"));
    //plotter->setStackPlots(false);
    appendVisualization(interface, plotter);
    connectSource("network/interfaces/" + interface + "/receiver/data");
    setPreferredItemHeight(80);
    return true;
}

void SM::Net::dataUpdated(const QString& source,
                          const Plasma::DataEngine::Data &data)
{
    QStringList splitted = source.split('/');

    if (splitted.length() < 4) {
        return;
    }

    QString interface = splitted[2];
    int index = (splitted[3] == "receiver") ? 0 : 1;

    if (!m_data.contains(interface)) {
        m_data[interface] = QList<double>() << -1 << -1;
    }

    m_data[interface][index] = qMax(0.0, data["value"].toDouble());

    if (!m_data[interface].contains(-1)) {

        SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(interface));
        if (plotter) {
            plotter->addSample(m_data[interface]);

            if (mode() == SM::Applet::Panel) {
                const double downstream = m_data[interface][0];
                const double upstream = m_data[interface][1];


                QString tooltip = QString::fromUtf8("<b>%1</b> <br /> ⇧ &nbsp; %2 <br />⇩ &nbsp; %3<br />");

                setToolTip(interface, tooltip.arg(plotter->title())
                .arg(KGlobal::locale()->formatByteSize(upstream*1024))
                .arg(KGlobal::locale()->formatByteSize(downstream*1024)));
            }
        }
        m_data[interface] = QList<double>() << -1 << -1;
    }
}

void SM::Net::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_model.clear();
    m_model.setHorizontalHeaderLabels(QStringList() << i18n("Network Interface"));
    QStandardItem *parentItem = m_model.invisibleRootItem();

    foreach (const QString& interface, m_interfaces) {
        QString ifname = interface.split('/')[2];
        QStandardItem *item1 = new QStandardItem(ifname);
        item1->setEditable(false);
        item1->setCheckable(true);
        item1->setData(interface);
        if (sources().contains(interface)) {
            item1->setCheckState(Qt::Checked);
        }
        parentItem->appendRow(QList<QStandardItem *>() << item1);
    }
    ui.treeView->setModel(&m_model);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->addPage(widget, i18n("Interfaces"), "network-workgroup");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void SM::Net::configAccepted()
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
    cg.writeEntry("interfaces", sources());

    double interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

#include "net.moc"
