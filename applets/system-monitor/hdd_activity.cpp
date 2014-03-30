/*
 *   Copyright (C) 2011, 2012 Shaun Reich <shaun.reich@kdemail.net>
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

#include "hdd_activity.h"
#include "monitoricon.h"
#include "plotter.h"

#include <QDebug>
#include <KConfigDialog>
#include <KColorUtils>

#include <QFileInfo>
#include <QGraphicsLinearLayout>

#include <Plasma/Meter>
#include <Plasma/Containment>
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>

/**
 * Examples of what the regexp has to handle...
 *
 * Note actually it's now set to an inclusive-only mode, far cleaner.
 *
 * Included items only:
 *
 *
 * RAID blocks (there *could* be more if not using mdadm, I think):
 *
 * disk/md<something>/Rate/rio
 * disk/md<something>/Rate/wio
 *
 * SATA disks:
 *
 * disk/sd<something>/Rate/rio
 * disk/sd<something>/Rate/wio
 *
 * IDE/PATA disks:
 *
 * disk/hd<something>/Rate/rio
 * disk/hd<something>/Rate/wio
 *
 * Does NOT include partitions, since I don't think many people will use that.
 *
 * RAID regexp handling needs further testing, I have a feeling it works just for me.
 *
 */
Hdd_Activity::Hdd_Activity(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args),
    m_regexp("disk/(?:md|sd|hd)[a-z|0-9]_.*/Rate/(?:rblk|wblk)")
{
    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

Hdd_Activity::~Hdd_Activity()
{
}

void Hdd_Activity::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    setEngine(dataEngine("systemmonitor"));
    setTitle(i18n("Disk Activity"));

    /* At the time this method is running, not all sources may be connected. */
    connect(engine(), SIGNAL(sourceAdded(QString)), this, SLOT(sourceChanged(QString)));
    connect(engine(), SIGNAL(sourceRemoved(QString)), this, SLOT(sourceChanged(QString)));

    foreach (const QString& source, engine()->sources()) {
        sourceChanged(source);
    }

    configChanged();
}

void Hdd_Activity::sourceChanged(const QString& name)
{
    //qDebug() << "######## sourceChanged name: " << name;
    //qDebug() << "###### regexp captures: " << m_regexp.capturedTexts();

    if (m_regexp.indexIn(name) != -1) {
        m_possibleHdds.append(name);
    }
}

void Hdd_Activity::sourcesChanged()
{
    configChanged();
}

void Hdd_Activity::dataUpdated(const QString& source, const Plasma::DataEngine::Data &data)
{


    const double value = data["value"].toDouble();
    QVector<double>& valueVector = m_data[source];

    if (valueVector.size() < 2) {
        valueVector.resize(2);
    }

    QString sneakySource = source;
    // we're interested in all source for a device which are
    // rblk and wblk. however, only 1 vis per that. so it wouldn't be unique and we'd only
    // get the values for rblk.
    // so add data to the hash, since we obtain the pair
    // on separate dataUpdated calls, so we'll need to map them
    if (sneakySource.endsWith("rblk")) {
        valueVector[0] = value;
    } else if (sneakySource.endsWith("wblk")) {
        valueVector[1] = value;
        //make the source *appear* to be a rblk.
        sneakySource.remove("wblk");
        sneakySource.append("rblk");
    }

    // we look up the visualization that is /...disk/rblk only. there is
    // not a rblk vis, so one just holds all data.
    SM::Plotter *plotter = qobject_cast<SM::Plotter*>(visualization(sneakySource));

    // plotter is invalid if we're e.g. switching monitored stuff
    if (!plotter) {
        return;
    }

    //only graph it if it's got both rblk and wblk
    if (valueVector.count() == 2) {
        QString read = KGlobal::locale()->formatNumber(valueVector.at(0), 1);
        QString write = KGlobal::locale()->formatNumber(valueVector.at(1), 1);

        //FIXME: allow plotter->addSample overload for QVector.
        plotter->addSample(valueVector.toList());

        if (mode() == SM::Applet::Panel) {
            const QString tooltip = QString("<tr><td>%1&nbsp;</td><td>rio: %2%</td><td>wio: %3</td></tr>")
                                    .arg(plotter->title()).arg(read).arg(write);

            setToolTip(source, tooltip);
        }
    }
}

void Hdd_Activity::createConfigurationInterface(KConfigDialog *parent)
{

    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_hddModel.clear();
    m_hddModel.setHorizontalHeaderLabels(QStringList() << i18n("Name"));
    QStandardItem *parentItem = m_hddModel.invisibleRootItem();

    foreach (const QString& hdd, m_possibleHdds) {
        if (m_regexp.indexIn(hdd) != -1) {
            if (hdd.endsWith("rblk")) {

                // so the user only sees 1 device (which includes both rblk/wblk
                QString trimmedHdd = hdd;
                trimmedHdd.remove("rblk");

                QStandardItem *item1 = new QStandardItem(trimmedHdd);
                item1->setEditable(false);
                item1->setCheckable(true);

                // store the pair of real sources (rblk, wblk) into data
                QStringList realSources;
                realSources.append(trimmedHdd + "rblk");
                realSources.append(trimmedHdd + "wblk");

                item1->setData(realSources);

                // but the behind the scenes still uses the source separation,
                // so be sure to use that.
                if (sources().contains(hdd)) {
                    item1->setCheckState(Qt::Checked);
                }

                parentItem->appendRow(QList<QStandardItem *>() << item1);
            }
        }
    }

    ui.treeView->setModel(&m_hddModel);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 1000.0);
    ui.intervalSpinBox->setSuffix(i18nc("second", " s"));
    parent->addPage(widget, i18n("Hard Disks"), "drive-harddisk");

    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void Hdd_Activity::configChanged()
{
    //qDebug() << "#### configChanged m_hdds:" << m_hdds;

    KConfigGroup cg = config();
    QStringList default_hdds = m_possibleHdds;

    // default to 2 seconds (2000 ms interval
    setInterval(cg.readEntry("interval", 2.0) * 1000.0);
    setSources(cg.readEntry("hdds", default_hdds));

    connectToEngine();
}

void Hdd_Activity::configAccepted()
{
    KConfigGroup cg = config();
    QStandardItem *parentItem = m_hddModel.invisibleRootItem();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            if (item->checkState() == Qt::Checked) {
                QStringList actualSources = item->data().toStringList();

                const QString& rblk = actualSources.at(0);
                const QString& wblk = actualSources.at(1);
                appendSource(rblk);
                appendSource(wblk);
            }
        }
    }

    cg.writeEntry("hdds", sources());

    uint interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

bool Hdd_Activity::addVisualization(const QString& source)
{
    QStringList splits = source.split('/');

    // 0 == "disk" 1 == "sde_(8:64)" 2 == "Rate" 3 == "rblk"
    Q_ASSERT(splits.count() == 4);

    // only monitor rblk for each device. we watch all sources/connect to them
    // but only need 1 vis per actual device (otherwise there'd be 1 for rblk, 1
    // for wblk, which isn't what I have in mind -sreich
    if (splits.at(3) == "rblk") {

        QString hdd = source;
        hdd = hdd.remove("rblk");

        //qDebug() << "#### ADD VIS hdd: " << hdd;

        SM::Plotter *plotter = new SM::Plotter(this);
        plotter->setTitle(hdd);

        // FIXME: localize properly..including units.
        // ksysguard just gives us 1024 KiB for sources, we need to convert for anything else.
        plotter->setUnit("KiB/s");

        // should be read, write, respectively
        plotter->setCustomPlots(QList<QColor>() << QColor("#0057AE") << QColor("#E20800"));

        appendVisualization(source, plotter);
        setPreferredItemHeight(80);
    }

    // if we return false here, it thinks we don't want
    // source updates for this source
    // which makes sense (since there's no vis). but not in
    // this case.
    return true;
}

#include "hdd_activity.moc"
