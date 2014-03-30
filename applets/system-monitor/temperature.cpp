/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
 *   Copyright (C) 2008 Marco Martin <notmart@gmail.com>
 *   Copyright (C) 2010 Michel Lafon-Puyo <michel.lafonpuyo@gmail.com>
 *   Copyright (C) 2011 Elvis Stansvik <elvstone@gmail.com>
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

#include "temperature.h"
#include <Plasma/Meter>
#include <Plasma/Containment>
#include <Plasma/Theme>
#include <KConfigDialog>
#include <KUnitConversion/Converter>
#include <KUnitConversion/Value>
#include <QGraphicsLinearLayout>
#include <QTimer>
#include <cmath>
#include "plotter.h"
#include "temperature-offset-delegate.h"

using namespace KUnitConversion;

Temperature::Temperature(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
    , m_tempModel(0)
    , m_rx(".*temp.*", Qt::CaseInsensitive)
{
    setHasConfigurationInterface(true);
    resize(215 + 20 + 23, 109 + 20 + 25);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_sourceTimer.setSingleShot(true);
    connect(&m_sourceTimer, SIGNAL(timeout()), this, SLOT(sourcesAdded()));
}

Temperature::~Temperature()
{
}

void Temperature::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("Temperature"));

    /* At the time this method is running, not all source may be connected. */
    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceAdded(QString)));
    foreach (const QString& source, engine()->sources()) {
        sourceAdded(source);
    }
}

void Temperature::configChanged()
{
    KConfigGroup cg = config();
    setInterval(cg.readEntry("interval", 2.0) * 1000.0);
    setSources(cg.readEntry("temps", m_sources.mid(0, 5)));
    connectToEngine();
}

void Temperature::sourceAdded(const QString& name)
{
    if (m_rx.indexIn(name) != -1) {
        //qDebug() << m_rx.cap(1);
        m_sources << name;
        if (!m_sourceTimer.isActive()) {
            m_sourceTimer.start(0);
        }
    }
}

void Temperature::sourcesAdded()
{
    configChanged();
}

void Temperature::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_tempModel.clear();
    m_tempModel.setHorizontalHeaderLabels(QStringList() << i18n("Sensor")
                                                        << i18n("Name")
                                                        << i18n("Offset"));

    QStandardItem *parentItem = m_tempModel.invisibleRootItem();
    foreach (const QString& temp, m_sources) {
        QStandardItem *item1 = new QStandardItem(temp);
        item1->setEditable(false);
        item1->setCheckable(true);
        if (sources().contains(temp)) {
            item1->setCheckState(Qt::Checked);
        }
        QStandardItem *item2 = new QStandardItem(temperatureTitle(temp));
        item2->setEditable(true);
        QStandardItem *item3 = new QStandardItem(
                KGlobal::locale()->formatNumber(temperatureOffset(temp), 1));
        item3->setEditable(true);
        parentItem->appendRow(QList<QStandardItem *>() << item1 << item2 << item3);
    }
    ui.treeView->setModel(&m_tempModel);
    ui.treeView->resizeColumnToContents(0);
    ui.treeView->setItemDelegateForColumn(2, new TemperatureOffsetDelegate());

    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Apply);
    parent->addPage(widget, i18n("Temperature"), "view-statistics");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void Temperature::configAccepted()
{
    KConfigGroup cg = config();
    KConfigGroup cgGlobal = globalConfig();
    QStandardItem *parentItem = m_tempModel.invisibleRootItem();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            cgGlobal.writeEntry(item->text(),
                                parentItem->child(i, 1)->text());
            cgGlobal.writeEntry(item->text() + "_offset", QString::number(
                                    parentItem->child(i, 2)->data(Qt::EditRole).toDouble(), 'f', 1));
            if (item->checkState() == Qt::Checked) {
                appendSource(item->text());
            }
        }
    }
    cg.writeEntry("temps", sources());
    uint interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

QString Temperature::temperatureTitle(const QString& source)
{
    KConfigGroup cg = globalConfig();
    return cg.readEntry(source, source.mid(source.lastIndexOf('/') + 1).replace('_',' '));
}

double Temperature::temperatureOffset(const QString& source)
{
    KConfigGroup cg = globalConfig();
    return cg.readEntry(source + "_offset", 0.0);
}

bool Temperature::addVisualization(const QString& source)
{
    Plasma::DataEngine *engine = dataEngine("systemmonitor");

    if (!engine) {
        return false;
    }

    SM::Plotter *plotter = new SM::Plotter(this);
    plotter->setTitle(temperatureTitle(source));
    plotter->setAnalog(mode() != SM::Applet::Panel);

    if (KGlobal::locale()->measureSystem() == KLocale::Metric) {
        plotter->setMinMax(0, 110);
        plotter->setUnit(QString::fromUtf8("°C"));
    } else {
        plotter->setMinMax(32, 230);
        plotter->setUnit(QString::fromUtf8("°F"));
    }
    appendVisualization(source, plotter);

    Plasma::DataEngine::Data data = engine->query(source);
    dataUpdated(source, data);
    setPreferredItemHeight(80);
    return true;
}

void Temperature::dataUpdated(const QString& source,
                              const Plasma::DataEngine::Data &data)
{
    if (!sources().contains(source)) {
        return;
    }
    SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(source));
    QString temp;
    QString unit = data["units"].toString();
    double doubleValue = data["value"].toDouble() + temperatureOffset(source);
    Value value = Value(doubleValue, unit);

    if (KGlobal::locale()->measureSystem() == KLocale::Metric) {
        value = value.convertTo(Celsius);
    } else {
        value = value.convertTo(Fahrenheit);
    }

    value.round(1);
    if (plotter) {
        plotter->addSample(QList<double>() << value.number());
    }

    temp = value.toSymbolString();

    if (mode() == SM::Applet::Panel) {
        setToolTip(source,
                   QString("<tr><td>%1</td><td>%2</td></tr>").arg(temperatureTitle(source)).arg(temp));
    }
}

#include "temperature.moc"
