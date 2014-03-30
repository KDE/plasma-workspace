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

#include "ram.h"
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>
#include <KConfigDialog>
#include <QTimer>
#include <QGraphicsLinearLayout>
#include "plotter.h"

/* All sources we are interested in. */
static const char phys_source[] = "mem/physical/application";
static const char swap_source[] = "mem/swap/used";

SM::Ram::Ram(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
{
    setHasConfigurationInterface(true);
    resize(234 + 20 + 23, 135 + 20 + 25);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

SM::Ram::~Ram()
{
}

void SM::Ram::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("RAM"));

    /* At the time this method is running, not all source may be connected. */
    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceAdded(QString)));
    foreach (const QString& source, engine()->sources()) {
        sourceAdded(source);
    }
}

void SM::Ram::configChanged()
{
    KConfigGroup cg = config();
    setInterval(cg.readEntry("interval", 2.0) * 1000.0);
    // sanity check
    QStringList memories = cg.readEntry("memories", m_memories);
    foreach (QString source, memories) {
        if (source != phys_source && source != swap_source)
            memories.removeAt(memories.indexOf(source));
    }
    setSources(memories);
    m_max.clear();
    connectToEngine();
}

void SM::Ram::sourceAdded(const QString& name)
{
    if ((name == phys_source || name == swap_source) && !m_memories.contains(name)) {
        m_memories << name;
        if (m_memories.count() == 2) {
            // all sources are ready
            QTimer::singleShot(0, this, SLOT(sourcesAdded()));
        }
    }
}

void SM::Ram::sourcesAdded()
{
    configChanged();
}

bool SM::Ram::addVisualization(const QString& source)
{
    QStringList l = source.split('/');
    if (l.count() < 3) {
        return false;
    }
    QString ram = l[1];

    SM::Plotter *plotter = new SM::Plotter(this);

    // 'ram' should be "physical" or "swap". I'm not aware of other values
    // for it, but who knows.
    if (ram == "physical") {
        ram = i18nc("noun, hardware, physical RAM/memory", "physical");
    } else if (ram == "swap") {
        ram = i18nc("noun, hardware, swap file/partition", "swap");
    }

    plotter->setTitle(ram);
    plotter->setUnit("B");

    appendVisualization(source, plotter);
    setPreferredItemHeight(80);

    return true;
}

double SM::Ram::preferredBinaryUnit()
{
    KLocale::BinaryUnitDialect binaryUnit = KGlobal::locale()->binaryUnitDialect();

    // this makes me feel all dirty inside. but it's the only way I could find
    // which will let us know what we should be scaling our graph by, independent
    // of how locale settings are configured.
    switch (binaryUnit) {
        case KLocale::IECBinaryDialect:
            //fallthrough
        case KLocale::JEDECBinaryDialect:
            return 1024;
            break;
        case KLocale::MetricBinaryDialect:
            return 1000;
            break;

        default:
            // being careful..I'm sure some genius will invent a new byte unit system ;-)
            Q_ASSERT_X(0, "preferredBinaryUnit", "invalid binary preference enum returned");
            return 0;
    }
}

QStringList SM::Ram::preferredUnitsList()
{
    QStringList units;
    KLocale::BinaryUnitDialect binaryUnit = KGlobal::locale()->binaryUnitDialect();
    switch (binaryUnit) {
        case KLocale::IECBinaryDialect:
            units << "B" << "KiB" << "MiB" << "GiB" << "TiB";
            break;
        case KLocale::JEDECBinaryDialect:
            units << "B" << "KB" << "MB" << "GB" << "TB";
            break;
        case KLocale::MetricBinaryDialect:
            units << "B" << "kB" << "MB" << "GB" << "TB";
            break;
        default:
            Q_ASSERT_X(0, "preferredBinaryUnit", "invalid binary preference enum returned");
    }

    return units;
}

void SM::Ram::dataUpdated(const QString& source, const Plasma::DataEngine::Data &data)
{
    SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(source));
    if (plotter) {
        /* A factor to convert from default units to bytes.
         * If units is not "KB", assume it is bytes.
         * NOTE: the dataengine refers to KB == 1024. so it's KiB as well.
         * Though keep in mind, KB does not imply 1024 and can be KB == 1000 as well.
         */
        const double preferredUnit = preferredBinaryUnit();
        const double factor = (data["units"].toString() == "KB") ? preferredUnit : 1.0;
        const double value_b = data["value"].toDouble() * factor;
        const double max_b = data["max"].toDouble() * factor;
        const QStringList units = preferredUnitsList();

        if (value_b > m_max[source]) {
            m_max[source] = max_b;
            plotter->setMinMax(0.0, max_b);
            qreal scale = 1.0;
            int i = 0;
            while (max_b / scale > factor && i < units.size()) {
                scale *= factor;
                ++i;
            }

            plotter->setUnit(units[i]);
            plotter->setScale(scale);
        }

        plotter->addSample(QList<double>() << value_b);
        QString temp = KGlobal::locale()->formatByteSize(value_b);

        if (mode() == SM::Applet::Panel) {
            setToolTip(source, QString("<tr><td>%1</td><td>%2</td><td>of</td><td>%3</td></tr>")
                                      .arg(plotter->title())
                                      .arg(temp)
                                      .arg(KGlobal::locale()->formatByteSize(m_max[source])));
        }
    }
}

void SM::Ram::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_model.clear();
    m_model.setHorizontalHeaderLabels(QStringList() << i18n("RAM"));
    QStandardItem *parentItem = m_model.invisibleRootItem();
    QRegExp rx("mem/(\\w+)/.*");
    QString ramName;

    foreach (const QString& ram, m_memories) {
        if (rx.indexIn(ram) != -1) {
            ramName = rx.cap(1);

            // 'ram' should be "physical" or "swap". I'm not aware of other values
            // for it, but who knows. (see also addVisualization)
            if (ramName == "physical") {
                ramName = i18nc("noun, hardware, physical RAM/memory", "physical");
            } else if (ramName == "swap") {
                ramName = i18nc("noun, hardware, swap file/partition", "swap");
            }

            QStandardItem *ramItem = new QStandardItem(ramName);
            ramItem->setEditable(false);
            ramItem->setCheckable(true);
            ramItem->setData(ram);

            if (sources().contains(ram)) {
                ramItem->setCheckState(Qt::Checked);
            }

            parentItem->appendRow(ramItem);
        }
    }

    ui.treeView->setModel(&m_model);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->addPage(widget, i18n("RAM"), "media-flash");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void SM::Ram::configAccepted()
{
    KConfigGroup cg = config();
    QStandardItem *parentItem = m_model.invisibleRootItem();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            if (item->checkState() == Qt::Checked) {
                // data() is the untranslated string
                // for use with sources
                appendSource(item->data().toString());
            }
        }
    }

    // note we write and read non-translated
    // version to config file.
    cg.writeEntry("memories", sources());

    double interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

#include "ram.moc"
