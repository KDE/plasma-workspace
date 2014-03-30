/*
 *   Copyright (C) 2007 Petri Damsten <damu@iki.fi>
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

#include "hdd.h"
#include "monitoricon.h"
#include <Plasma/Meter>
#include <Plasma/Containment>
#include <Plasma/Theme>
#include <Plasma/ToolTipManager>
#include <KConfigDialog>
#include <KColorUtils>
#include <QFileInfo>
#include <QGraphicsLinearLayout>

Hdd::Hdd(QObject *parent, const QVariantList &args)
    : SM::Applet(parent, args)
{
    setHasConfigurationInterface(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(themeChanged()));
}

Hdd::~Hdd()
{
}

void Hdd::init()
{
    KGlobal::locale()->insertCatalog("plasma_applet_system-monitor");
    QString predicateString("IS StorageVolume");
    setEngine(dataEngine("soliddevice"));

    setTitle(i18n("Disk Space"));

    configChanged();
}

void Hdd::configChanged()
{
    KConfigGroup cg = config();
    QStringList sources = cg.readEntry("uuids", mounted());
    setSources(sources);
    setInterval(cg.readEntry("interval", 2) * 60 * 1000);
    connectToEngine();
}

QStringList Hdd::mounted()
{
    Plasma::DataEngine::Data data;
    QString predicate("IS StorageVolume");
    QStringList result;

    foreach (const QString& uuid, engine()->query(predicate)[predicate].toStringList()) {
        if (!isValidDevice(uuid, &data)) {
            continue;
        }
        if (data["Accessible"].toBool()) {
            result << uuid;
        }
    }
    return result;
}

void Hdd::createConfigurationInterface(KConfigDialog *parent)
{
    QWidget *widget = new QWidget();
    ui.setupUi(widget);
    m_hddModel.clear();
    m_hddModel.setHorizontalHeaderLabels(QStringList() << i18n("Mount Point")
                                                       << i18n("Name"));
    QStandardItem *parentItem = m_hddModel.invisibleRootItem();
    Plasma::DataEngine::Data data;
    QString predicateString("IS StorageVolume");

    foreach (const QString& uuid, engine()->query(predicateString)[predicateString].toStringList()) {
        if (!isValidDevice(uuid, &data)) {
            continue;
        }
        QStandardItem *item1 = new QStandardItem(filePath(data));
        item1->setEditable(false);
        item1->setCheckable(true);
        item1->setData(uuid);
        if (sources().contains(uuid)) {
            item1->setCheckState(Qt::Checked);
        }
        QStandardItem *item2 = new QStandardItem(hddTitle(uuid, data));
        item2->setData(guessHddTitle(data));
        item2->setEditable(true);
        parentItem->appendRow(QList<QStandardItem *>() << item1 << item2);
    }
    ui.treeView->setModel(&m_hddModel);
    ui.treeView->resizeColumnToContents(0);
    ui.intervalSpinBox->setValue(interval() / 60 / 1000);
    ui.intervalSpinBox->setSuffix(ki18np(" minute", " minutes"));

    parent->addPage(widget, i18n("Partitions"), "drive-harddisk");
    connect(parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()));
    connect(parent, SIGNAL(okClicked()), this, SLOT(configAccepted()));
    connect(ui.treeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(settingsModified()));
    connect(ui.intervalSpinBox, SIGNAL(valueChanged(QString)), parent, SLOT(settingsModified()));
}

void Hdd::configAccepted()
{
    KConfigGroup cg = config();
    KConfigGroup cgGlobal = globalConfig();
    QStandardItem *parentItem = m_hddModel.invisibleRootItem();

    clear();

    for (int i = 0; i < parentItem->rowCount(); ++i) {
        QStandardItem *item = parentItem->child(i, 0);
        if (item) {
            QStandardItem *child = parentItem->child(i, 1);
            if (child->text() != child->data().toString()) {
                cgGlobal.writeEntry(item->data().toString(), child->text());
            }
            if (item->checkState() == Qt::Checked) {
                appendSource(item->data().toString());
            }
        }
    }
    cg.writeEntry("uuids", sources());

    uint interval = ui.intervalSpinBox->value();
    cg.writeEntry("interval", interval);

    emit configNeedsSaving();
}

QString Hdd::hddTitle(const QString& uuid, const Plasma::DataEngine::Data &data)
{
    KConfigGroup cg = globalConfig();
    QString label = cg.readEntry(uuid, "");

    if (label.isEmpty()) {
        label = guessHddTitle(data);
    }
    return label;
}

QString Hdd::guessHddTitle(const Plasma::DataEngine::Data &data)
{
    QString label = data["Label"].toString();
    if (label.isEmpty()) {
        QString path = data["File Path"].toString();
        if (path == "/")
            return i18nc("the root filesystem", "root");
        QFileInfo fi(path);
        label = fi.fileName();
        if (label.isEmpty()) {
            label = data["Device"].toString();
            if (label.isEmpty()) {
                label = i18n("Unknown filesystem");
            }
        }
    }
    return label;
}

QString Hdd::filePath(const Plasma::DataEngine::Data &data)
{
    QString label = data["File Path"].toString();
    QVariant accessible = data["Accessible"];
    if (accessible.isValid()) {
        if (accessible.canConvert(QVariant::Bool)) {
            if (!accessible.toBool()) {
                label = i18nc("hard disk label (not mounted or accessible)",
                              "%1 (not accessible)", label);
            }
        }
    }
    return label;
}

bool Hdd::addVisualization(const QString& source)
{
    Plasma::Meter *w;
    Plasma::DataEngine *engine = dataEngine("soliddevice");
    Plasma::DataEngine::Data data;

    if (!engine) {
        return false;
    }
    if (!isValidDevice(source, &data)) {
        // do not try to show hard drives and swap partitions.
        return false;
    }
    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout(Qt::Horizontal);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(5);

    w = new Plasma::Meter(this);
    w->setMeterType(Plasma::Meter::BarMeterHorizontal);
    if (mode() != SM::Applet::Panel) {
        MonitorIcon *icon = new MonitorIcon(this);
        m_icons.insert(source, icon);
        icon->setImage("drive-harddisk");
        if (data["Accessible"].toBool()) {
            QStringList overlays;
            overlays << QString("emblem-mounted");
            icon->setOverlays(overlays);
        }
        layout->addItem(icon);
    } else {
        w->setSvg("system-monitor/hdd_panel");
    }
    w->setLabel(0, hddTitle(source, data));
    w->setLabelAlignment(0, Qt::AlignVCenter | Qt::AlignLeft);
    w->setLabelAlignment(1, Qt::AlignVCenter | Qt::AlignRight);
    w->setLabelAlignment(2, Qt::AlignVCenter | Qt::AlignCenter);
    w->setMaximum(data["Size"].toULongLong() / (1024 * 1024));
    applyTheme(w);
    appendVisualization(source, w);
    layout->addItem(w);
    mainLayout()->addItem(layout);
    dataUpdated(source, data);
    setPreferredItemHeight(layout->preferredSize().height());

    QString disk = data["Parent UDI"].toString();

    m_diskMap[disk] << w;
    if (!connectedSources().contains(disk)) {
        data = engine->query(disk);
        dataUpdated(disk, data);
        connectSource(disk);
    }
    return true;
}

void Hdd::applyTheme(Plasma::Meter *w)
{
    if (!w) {
        return;
    }

    Plasma::Theme* theme = Plasma::Theme::defaultTheme();
    QColor text = theme->color(Plasma::Theme::TextColor);
    QColor bg = theme->color(Plasma::Theme::BackgroundColor);
    QColor darkerText = KColorUtils::tint(text, bg, 0.4);
    w->setLabelColor(0, text);
    w->setLabelColor(1, darkerText);
    w->setLabelColor(2, darkerText);
    QFont font = theme->font(Plasma::Theme::DefaultFont);
    font.setPointSize(9);
    w->setLabelFont(0, font);
    font.setPointSizeF(7.5);
    w->setLabelFont(1, font);
    w->setLabelFont(2, font);
}

void Hdd::themeChanged()
{
    foreach (const QString& source, connectedSources()) {
        applyTheme(qobject_cast<Plasma::Meter*>(visualization(source)));
    }
}

void Hdd::deleteVisualizations()
{
    foreach(MonitorIcon * icon, m_icons) {
        delete(icon);
    }

    m_icons.clear();

    Applet::deleteVisualizations();
    m_diskMap.clear();
}

bool Hdd::isValidDevice(const QString& uuid, Plasma::DataEngine::Data* data)
{
    Plasma::DataEngine *engine = dataEngine("soliddevice");
    if (engine) {
        *data = engine->query(uuid);
        /*
        qDebug() << uuid << data->value("Device").toString() <<
                            data->value("Usage").toString() <<
                            data->value("File System Type").toString() <<
                            data->value("Size").toString();
        */
        if ((data->value("Usage").toString() != i18n("File System") &&
             data->value("Usage").toString() != i18n("Raid")) ||
            data->value("File System Type").toString() == "swap") {
            QStringList list = sources();
            list.removeAll(uuid);
            setSources(list);
            return false;
        }
        return true;
    }
    return false;
}

void Hdd::dataUpdated(const QString& source,
                      const Plasma::DataEngine::Data &data)
{
    if (m_diskMap.keys().contains(source) && mode() != SM::Applet::Panel) {
        if (data.keys().contains("Temperature")) {
            QList<Plasma::Meter *> widgets = m_diskMap[source];
            foreach (Plasma::Meter *w, widgets) {
                w->setLabel(2, QString("%1\xb0%2").arg(data["Temperature"].toString())
                                                  .arg(data["Temperature Unit"].toString()));
            }
        }
    } else {
        Plasma::Meter *w = qobject_cast<Plasma::Meter *>(visualization(source));
        if (!w) {
            return;
        }
        qulonglong size = qulonglong(data["Size"].toULongLong());
        qlonglong availBytes = 0;
        QVariant freeSpace = data["Free Space"];
        if (freeSpace.isValid()) {
            if (freeSpace.canConvert(QVariant::LongLong)) {
                availBytes = qlonglong(freeSpace.toLongLong());
                w->setValue((size / (1024 * 1024)) - (availBytes / (1024 * 1024)));
            }
        }
        else {
            w->setValue(0);
        }
        if (mode() != SM::Applet::Panel) {
            w->setLabel(1, KGlobal::locale()->formatByteSize(availBytes));
            QStringList overlays;
            if (data["Accessible"].toBool()) {
                overlays << "emblem-mounted";
            }
            m_icons[source]->setOverlays(overlays);
        } else {
            setToolTip(source, QString("<tr><td>%1</td><td>%2</td><td>/</td><td>%3</td></tr>")
                                      .arg(w->label(0))
                                      .arg(KGlobal::locale()->formatByteSize(availBytes))
                                      .arg(KGlobal::locale()->formatByteSize(size)));
        }
    }
}

#include "hdd.moc"
