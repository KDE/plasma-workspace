/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontList.h"
#include "Fc.h"
#include "FcEngine.h"
#include "FontInstInterface.h"
#include "GroupList.h"
#include "KfiConstants.h"
#include "XmlStrings.h"
#include <KColorScheme>
#include <KIconLoader>
#include <KMessageBox>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QDrag>
#include <QDropEvent>
#include <QFile>
#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPixmap>
#include <QProcess>
#include <QTimer>
#include <stdlib.h>
#include <unistd.h>
#include <utime.h>

namespace KFI
{
const QStringList CFontList::fontMimeTypes(QStringList() << "font/ttf"
                                                         << "font/otf"
                                                         << "font/collection"
                                                         << "application/x-font-ttf"
                                                         << "application/x-font-otf"
                                                         << "application/x-font-type1"
                                                         << "application/x-font-pcf"
                                                         << "application/x-font-bdf"
                                                         << "application/vnd.kde.fontspackage");

static const int constMaxSlowed = 250;

static void decompose(const QString &name, QString &family, QString &style)
{
    int commaPos = name.lastIndexOf(',');

    family = -1 == commaPos ? name : name.left(commaPos);
    style = -1 == commaPos ? KFI_WEIGHT_REGULAR : name.mid(commaPos + 2);
}

static void addFont(CFontItem *font,
                    CJobRunner::ItemList &urls,
                    QStringList &fontNames,
                    QSet<Misc::TFont> *fonts,
                    QSet<CFontItem *> &usedFonts,
                    bool getEnabled,
                    bool getDisabled)
{
    if (!usedFonts.contains(font) && ((getEnabled && font->isEnabled()) || (getDisabled && !font->isEnabled()))) {
        urls.append(CJobRunner::Item(font->url(), font->name(), !font->isEnabled()));
        fontNames.append(font->name());
        usedFonts.insert(font);
        if (fonts) {
            fonts->insert(Misc::TFont(font->family(), font->styleInfo()));
        }
    }
}

static QString replaceEnvVar(const QString &text)
{
    QString mod(text);
    int endPos(text.indexOf('/'));

    if (endPos == -1) {
        endPos = text.length() - 1;
    } else {
        endPos--;
    }

    if (endPos > 0) {
        QString envVar(text.mid(1, endPos));

        const char *val = getenv(envVar.toLatin1().constData());

        if (val) {
            mod = Misc::fileSyntax(QFile::decodeName(val) + mod.mid(endPos + 1));
        }
    }

    return mod;
}

//
// Convert from list such as:
//
//    Arial
//    Arial, Bold
//    Courier
//    Times
//    Times, Italic
//
// To:
//
//    Arial (Regular, Bold)
//    Coutier
//    Times (Regular, Italic)
QStringList CFontList::compact(const QStringList &fonts)
{
    QString lastFamily, entry;
    QStringList::ConstIterator it(fonts.begin()), end(fonts.end());
    QStringList compacted;
    QSet<QString> usedStyles;

    for (; it != end; ++it) {
        QString family, style;

        decompose(*it, family, style);

        if (family != lastFamily) {
            usedStyles.clear();
            if (entry.length()) {
                entry += ')';
                compacted.append(entry);
            }
            entry = QString(family + " (");
            lastFamily = family;
        }
        if (!usedStyles.contains(style)) {
            usedStyles.clear();
            if (entry.length() && '(' != entry[entry.length() - 1]) {
                entry += ", ";
            }
            entry += style;
            usedStyles.insert(style);
        }
    }

    if (entry.length()) {
        entry += ')';
        compacted.append(entry);
    }

    return compacted;
}

QString capitaliseFoundry(const QString &foundry)
{
    QString f(foundry.toLower());

    if (f == QLatin1String("ibm")) {
        return QLatin1String("IBM");
    } else if (f == QLatin1String("urw")) {
        return QLatin1String("URW");
    } else if (f == QLatin1String("itc")) {
        return QLatin1String("ITC");
    } else if (f == QLatin1String("nec")) {
        return QLatin1String("NEC");
    } else if (f == QLatin1String("b&h")) {
        return QLatin1String("B&H");
    } else if (f == QLatin1String("dec")) {
        return QLatin1String("DEC");
    } else {
        QChar *ch(f.data());
        int len(f.length());
        bool isSpace(true);

        while (len--) {
            if (isSpace) {
                *ch = ch->toUpper();
            }

            isSpace = ch->isSpace();
            ++ch;
        }
    }

    return f;
}

inline bool isSysFolder(const QString &sect)
{
    return i18n(KFI_KIO_FONTS_SYS) == sect || KFI_KIO_FONTS_SYS == sect;
}

CFontItem::CFontItem(CFontModelItem *p, const Style &s, bool sys)
    : CFontModelItem(p)
    , m_styleName(FC::createStyleName(s.value()))
    , m_style(s)
{
    refresh();
    if (!Misc::root()) {
        setIsSystem(sys);
    }
}

void CFontItem::refresh()
{
    FileCont::ConstIterator it(m_style.files().begin()), end(m_style.files().end());

    m_enabled = false;
    for (; it != end; ++it) {
        if (!Misc::isHidden(Misc::getFile((*it).path()))) {
            m_enabled = true;
            break;
        }
    }
}

CFamilyItem::CFamilyItem(CFontList &p, const Family &f, bool sys)
    : CFontModelItem(nullptr)
    , m_status(ENABLED)
    , m_realStatus(ENABLED)
    , m_regularFont(nullptr)
    , m_parent(p)
{
    m_name = f.name();
    addFonts(f.styles(), sys);
    // updateStatus();
}

CFamilyItem::~CFamilyItem()
{
    qDeleteAll(m_fonts);
    m_fonts.clear();
}

bool CFamilyItem::addFonts(const StyleCont &styles, bool sys)
{
    StyleCont::ConstIterator it(styles.begin()), end(styles.end());
    bool modified = false;

    for (; it != end; ++it) {
        CFontItem *font = findFont((*it).value(), sys);

        if (!font) {
            // New font style!
            m_fonts.append(new CFontItem(this, *it, sys));
            modified = true;
        } else {
            int before = (*it).files().size();

            font->addFiles((*it).files());

            if ((*it).files().size() != before) {
                modified = true;
                font->refresh();
            }
        }
    }
    return modified;
}

CFontItem *CFamilyItem::findFont(quint32 style, bool sys)
{
    CFontItemCont::ConstIterator fIt(m_fonts.begin()), fEnd(m_fonts.end());

    for (; fIt != fEnd; ++fIt) {
        if ((*(*fIt)).styleInfo() == style && (*(*fIt)).isSystem() == sys) {
            return (*fIt);
        }
    }

    return nullptr;
}

void CFamilyItem::getFoundries(QSet<QString> &foundries) const
{
    CFontItemCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());

    for (; it != end; ++it) {
        FileCont::ConstIterator fIt((*it)->files().begin()), fEnd((*it)->files().end());

        for (; fIt != fEnd; ++fIt) {
            if (!(*fIt).foundry().isEmpty()) {
                foundries.insert(capitaliseFoundry((*fIt).foundry()));
            }
        }
    }
}

bool CFamilyItem::usable(const CFontItem *font, bool root)
{
    return (root || (font->isSystem() && m_parent.allowSys()) || (!font->isSystem() && m_parent.allowUser()));
}

void CFamilyItem::addFont(CFontItem *font, bool update)
{
    m_fonts.append(font);
    if (update) {
        updateStatus();
        updateRegularFont(font);
    }
}

void CFamilyItem::removeFont(CFontItem *font, bool update)
{
    m_fonts.removeAll(font);
    if (update) {
        updateStatus();
    }
    if (m_regularFont == font) {
        m_regularFont = nullptr;
        if (update) {
            updateRegularFont(nullptr);
        }
    }
    delete font;
}

void CFamilyItem::refresh()
{
    updateStatus();
    m_regularFont = nullptr;
    updateRegularFont(nullptr);
}

bool CFamilyItem::updateStatus()
{
    bool root(Misc::root());
    EStatus oldStatus(m_status);
    CFontItemCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());
    int en(0), dis(0), allEn(0), allDis(0);
    bool oldSys(isSystem()), sys(false);

    m_fontCount = 0;
    for (; it != end; ++it) {
        if (usable(*it, root)) {
            if ((*it)->isEnabled()) {
                en++;
            } else {
                dis++;
            }
            if (!sys) {
                sys = (*it)->isSystem();
            }
            m_fontCount++;
        } else if ((*it)->isEnabled()) {
            allEn++;
        } else {
            allDis++;
        }
    }

    allEn += en;
    allDis += dis;

    m_status = en && dis ? PARTIAL : en ? ENABLED : DISABLED;

    m_realStatus = allEn && allDis ? PARTIAL : allEn ? ENABLED : DISABLED;

    if (!root) {
        setIsSystem(sys);
    }

    return m_status != oldStatus || isSystem() != oldSys;
}

bool CFamilyItem::updateRegularFont(CFontItem *font)
{
    static const quint32 constRegular = FC::createStyleVal(FC_WEIGHT_REGULAR, KFI_FC_WIDTH_NORMAL, FC_SLANT_ROMAN);

    CFontItem *oldFont(m_regularFont);
    bool root(Misc::root());

    if (font && usable(font, root)) {
        if (m_regularFont) {
            int regDiff = abs((long)(m_regularFont->styleInfo() - constRegular)), fontDiff = abs((long)(font->styleInfo() - constRegular));

            if (fontDiff < regDiff) {
                m_regularFont = font;
            }
        } else {
            m_regularFont = font;
        }
    } else // This case happens when the regular font is deleted...
    {
        CFontItemCont::ConstIterator it(m_fonts.begin()), end(m_fonts.end());
        quint32 current = 0x0FFFFFFF;

        for (; it != end; ++it) {
            if (usable(*it, root)) {
                quint32 diff = abs((long)((*it)->styleInfo() - constRegular));

                if (diff < current) {
                    m_regularFont = (*it);
                    current = diff;
                }
            }
        }
    }

    return oldFont != m_regularFont;
}

CFontList::CFontList(QWidget *parent)
    : QAbstractItemModel(parent)
    , m_allowSys(true)
    , m_allowUser(true)
    , m_slowUpdates(false)
{
    FontInst::registerTypes();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(QLatin1String(OrgKdeFontinstInterface::staticInterfaceName()),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForOwnerChange,
                                                           this);

    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &CFontList::dbusServiceOwnerChanged);
    connect(CJobRunner::dbus(), &OrgKdeFontinstInterface::fontsAdded, this, &CFontList::fontsAdded);
    connect(CJobRunner::dbus(), &OrgKdeFontinstInterface::fontsRemoved, this, &CFontList::fontsRemoved);
    connect(CJobRunner::dbus(), &OrgKdeFontinstInterface::fontList, this, &CFontList::fontList);
}

CFontList::~CFontList()
{
    qDeleteAll(m_families);
    m_families.clear();
    m_familyHash.clear();
}

void CFontList::dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to)
{
    Q_UNUSED(from);
    Q_UNUSED(to);

    if (name == QLatin1String(OrgKdeFontinstInterface::staticInterfaceName())) {
        load();
    }
}

void CFontList::fontList(int pid, const QList<KFI::Families> &families)
{
    //  printf("**** fontList:%d/%d  %d\n", pid, getpid(), families.count());

    if (pid == getpid()) {
        QList<KFI::Families>::ConstIterator it(families.begin()), end(families.end());
        int count(families.size());

        for (int i = 0; it != end; ++it, ++i) {
            fontsAdded(*it);
            Q_EMIT listingPercent(i * 100 / count);
        }
        Q_EMIT listingPercent(100);
    }
}

void CFontList::unsetSlowUpdates()
{
    setSlowUpdates(false);
}

void CFontList::load()
{
    for (int t = 0; t < NUM_MSGS_TYPES; ++t) {
        for (int f = 0; f < FontInst::FOLDER_COUNT; ++f) {
            m_slowedMsgs[t][f].clear();
        }
    }

    setSlowUpdates(false);

    Q_EMIT layoutAboutToBeChanged();
    //     beginRemoveRows(QModelIndex(), 0, m_families.count());
    m_families.clear();
    m_familyHash.clear();
    //     endRemoveRows();
    Q_EMIT layoutChanged();
    Q_EMIT listingPercent(0);

    CJobRunner::startDbusService();
    CJobRunner::dbus()->list(FontInst::SYS_MASK | FontInst::USR_MASK, getpid());
}

void CFontList::setSlowUpdates(bool slow)
{
    if (m_slowUpdates != slow) {
        if (!slow) {
            for (int i = 0; i < FontInst::FOLDER_COUNT; ++i) {
                actionSlowedUpdates(i == FontInst::FOLDER_SYS);
            }
        }
        m_slowUpdates = slow;
    }
}

int CFontList::columnCount(const QModelIndex &) const
{
    return NUM_COLS;
}

QVariant CFontList::data(const QModelIndex &, int) const
{
    return QVariant();
}

Qt::ItemFlags CFontList::flags(const QModelIndex &index) const
{
    return !index.isValid() ? Qt::ItemIsEnabled | Qt::ItemIsDropEnabled
                            : Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

Qt::DropActions CFontList::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData *CFontList::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;
    QModelIndexList::ConstIterator it(indexes.begin()), end(indexes.end());
    QSet<QString> families;
    QDataStream ds(&encodedData, QIODevice::WriteOnly);

    for (; it != end; ++it) {
        if ((*it).isValid()) {
            if ((static_cast<CFontModelItem *>((*it).internalPointer()))->isFont()) {
                CFontItem *font = static_cast<CFontItem *>((*it).internalPointer());

                families.insert(font->family());
            } else {
                CFamilyItem *fam = static_cast<CFamilyItem *>((*it).internalPointer());

                families.insert(fam->name());
            }
        }
    }

    ds << families;
    mimeData->setData(KFI_FONT_DRAG_MIME, encodedData);
    return mimeData;
}

QStringList CFontList::mimeTypes() const
{
    QStringList types;

    types << "text/uri-list";
    return types;
}

QVariant CFontList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case COL_FONT:
                return i18n("Font");
            case COL_STATUS:
                return i18n("Status");
            default:
                break;
            }
            break;
            //             case Qt::DecorationRole:
            //                 if(COL_STATUS==section)
            //                     return QIcon::fromTheme("fontstatus");
            //                 break;
        case Qt::TextAlignmentRole:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case Qt::ToolTipRole:
            if (COL_STATUS == section) {
                return i18n(
                    "This column shows the status of the font family, and of the "
                    "individual font styles.");
            }
            break;
        case Qt::WhatsThisRole:
            return whatsThis();
        default:
            break;
        }
    }

    return QVariant();
}

QModelIndex CFontList::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) // Then font...
    {
        CFamilyItem *fam = static_cast<CFamilyItem *>(parent.internalPointer());

        if (row < fam->fonts().count()) {
            return createIndex(row, column, fam->fonts().at(row));
        }
    } else // Family....
        if (row < m_families.count()) {
        return createIndex(row, column, m_families.at(row));
    }

    return QModelIndex();
}

QModelIndex CFontList::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    CFontModelItem *mi = static_cast<CFontModelItem *>(index.internalPointer());

    if (mi->isFamily()) {
        return QModelIndex();
    } else {
        CFontItem *font = static_cast<CFontItem *>(index.internalPointer());

        return createIndex(m_families.indexOf(((CFamilyItem *)font->parent())), 0, font->parent());
    }
}

int CFontList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        CFontModelItem *mi = static_cast<CFontModelItem *>(parent.internalPointer());

        if (mi->isFont()) {
            return 0;
        }

        CFamilyItem *fam = static_cast<CFamilyItem *>(parent.internalPointer());

        return fam->fonts().count();
    } else {
        return m_families.count();
    }
}

void CFontList::refresh(bool allowSys, bool allowUser)
{
    m_allowSys = allowSys;
    m_allowUser = allowUser;
    CFamilyItemCont::ConstIterator it(m_families.begin()), end(m_families.end());

    for (; it != end; ++it) {
        (*it)->refresh();
    }
}

void CFontList::getFamilyStats(QSet<QString> &enabled, QSet<QString> &disabled, QSet<QString> &partial)
{
    CFamilyItemCont::ConstIterator it(m_families.begin()), end(m_families.end());

    for (; it != end; ++it) {
        switch ((*it)->realStatus()) {
        case CFamilyItem::ENABLED:
            enabled.insert((*it)->name());
            break;
        case CFamilyItem::PARTIAL:
            partial.insert((*it)->name());
            break;
        case CFamilyItem::DISABLED:
            disabled.insert((*it)->name());
            break;
        }
    }
}

void CFontList::getFoundries(QSet<QString> &foundries) const
{
    CFamilyItemCont::ConstIterator it(m_families.begin()), end(m_families.end());

    for (; it != end; ++it) {
        (*it)->getFoundries(foundries);
    }
}

QString CFontList::whatsThis() const
{
    return i18n(
        "<p>This list shows your installed fonts. The fonts are grouped by family, and the"
        " number in square brackets represents the number of styles in which the family is"
        " available. e.g.</p>"
        "<ul>"
        "<li>Times [4]"
        "<ul><li>Regular</li>"
        "<li>Bold</li>"
        "<li>Bold Italic</li>"
        "<li>Italic</li>"
        "</ul>"
        "</li>"
        "</ul>");
}

void CFontList::fontsAdded(const KFI::Families &families)
{
    // printf("**** FONT ADDED:%d\n", families.items.count());
    if (m_slowUpdates) {
        storeSlowedMessage(families, MSG_ADD);
    } else {
        addFonts(families.items, families.isSystem && !Misc::root());
    }
}

void CFontList::fontsRemoved(const KFI::Families &families)
{
    // printf("**** FONT REMOVED:%d\n", families.items.count());
    if (m_slowUpdates) {
        storeSlowedMessage(families, MSG_DEL);
    } else {
        removeFonts(families.items, families.isSystem && !Misc::root());
    }
}

void CFontList::storeSlowedMessage(const Families &families, EMsgType type)
{
    int folder = families.isSystem ? FontInst::FOLDER_SYS : FontInst::FOLDER_USER;
    bool playOld = false;

    for (int i = 0; i < NUM_MSGS_TYPES && !playOld; ++i) {
        if (m_slowedMsgs[i][folder].count() > constMaxSlowed) {
            playOld = true;
        }
    }

    if (playOld) {
        actionSlowedUpdates(families.isSystem);
    }

    FamilyCont::ConstIterator family(families.items.begin()), fend(families.items.end());

    for (; family != fend; ++family) {
        FamilyCont::ConstIterator f = m_slowedMsgs[type][folder].find(*family);

        if (f != m_slowedMsgs[type][folder].end()) {
            StyleCont::ConstIterator style((*family).styles().begin()), send((*family).styles().end());

            for (; style != send; ++style) {
                StyleCont::ConstIterator st = (*f).styles().find(*style);

                if (st == (*f).styles().end()) {
                    (*f).add(*style);
                } else {
                    (*st).addFiles((*style).files());
                }
            }
        } else {
            m_slowedMsgs[type][folder].insert(*family);
        }
    }
}

void CFontList::actionSlowedUpdates(bool sys)
{
    int folder = sys ? FontInst::FOLDER_SYS : FontInst::FOLDER_USER;

    for (int i = 0; i < NUM_MSGS_TYPES; ++i) {
        if (!m_slowedMsgs[i][folder].isEmpty()) {
            if (MSG_ADD == i) {
                addFonts(m_slowedMsgs[i][folder], sys && !Misc::root());
            } else {
                removeFonts(m_slowedMsgs[i][folder], sys && !Misc::root());
            }
            m_slowedMsgs[i][folder].clear();
        }
    }
}

void CFontList::addFonts(const FamilyCont &families, bool sys)
{
    //     bool emitLayout=!m_slowUpdates || m_families.isEmpty();
    //
    //     if(emitLayout)
    //         emit layoutAboutToBeChanged();

    FamilyCont::ConstIterator family(families.begin()), end(families.end());
    int famRowFrom = m_families.count();
    QSet<CFamilyItem *> modifiedFamilies;

    for (; family != end; ++family) {
        if ((*family).styles().count() > 0) {
            CFamilyItem *famItem = findFamily((*family).name());

            if (famItem) {
                int rowFrom = famItem->fonts().count();
                if (famItem->addFonts((*family).styles(), sys)) {
                    int rowTo = famItem->fonts().count();

                    if (rowTo != rowFrom) {
                        beginInsertRows(createIndex(famItem->rowNumber(), 0, famItem), rowFrom, rowTo);
                        endInsertRows();
                    }

                    modifiedFamilies.insert(famItem);
                }
            } else {
                famItem = new CFamilyItem(*this, *family, sys);
                m_families.append(famItem);
                m_familyHash[famItem->name()] = famItem;
                modifiedFamilies.insert(famItem);
            }
        }
    }

    int famRowTo = m_families.count();
    if (famRowTo != famRowFrom) {
        beginInsertRows(QModelIndex(), famRowFrom, famRowTo);
        endInsertRows();
    }

    if (!modifiedFamilies.isEmpty()) {
        QSet<CFamilyItem *>::Iterator it(modifiedFamilies.begin()), end(modifiedFamilies.end());

        for (; it != end; ++it) {
            (*it)->refresh();
        }
    }

    //     if(emitLayout)
    //         emit layoutChanged();
}

void CFontList::removeFonts(const FamilyCont &families, bool sys)
{
    //     if(!m_slowUpdates)
    //         emit layoutAboutToBeChanged();

    FamilyCont::ConstIterator family(families.begin()), end(families.end());
    QSet<CFamilyItem *> modifiedFamilies;

    for (; family != end; ++family) {
        if ((*family).styles().count() > 0) {
            CFamilyItem *famItem = findFamily((*family).name());

            if (famItem) {
                StyleCont::ConstIterator it((*family).styles().begin()), end((*family).styles().end());

                for (; it != end; ++it) {
                    CFontItem *fontItem = famItem->findFont((*it).value(), sys);

                    if (fontItem) {
                        int before = fontItem->files().count();

                        fontItem->removeFiles((*it).files());

                        if (fontItem->files().count() != before) {
                            if (fontItem->files().isEmpty()) {
                                int row = -1;
                                if (1 != famItem->fonts().count()) {
                                    row = fontItem->rowNumber();
                                    beginRemoveRows(createIndex(famItem->rowNumber(), 0, famItem), row, row);
                                }
                                famItem->removeFont(fontItem, false);
                                if (-1 != row) {
                                    endRemoveRows();
                                }
                            } else {
                                fontItem->refresh();
                            }
                        }
                    }
                }

                if (famItem->fonts().isEmpty()) {
                    int row = famItem->rowNumber();
                    beginRemoveRows(QModelIndex(), row, row);
                    m_familyHash.remove(famItem->name());
                    m_families.removeAt(row);
                    endRemoveRows();
                } else {
                    modifiedFamilies.insert(famItem);
                }
            }
        }
    }

    if (!modifiedFamilies.isEmpty()) {
        QSet<CFamilyItem *>::Iterator it(modifiedFamilies.begin()), end(modifiedFamilies.end());

        for (; it != end; ++it) {
            (*it)->refresh();
        }
    }

    //     if(!m_slowUpdates)
    //         emit layoutChanged();
}

CFamilyItem *CFontList::findFamily(const QString &familyName)
{
    CFamilyItemHash::Iterator it = m_familyHash.find(familyName);

    return it == m_familyHash.end() ? nullptr : *it;
}

inline bool matchString(const QString &str, const QString &pattern)
{
    return pattern.isEmpty() || -1 != str.indexOf(pattern, 0, Qt::CaseInsensitive);
}

CFontListSortFilterProxy::CFontListSortFilterProxy(QObject *parent, QAbstractItemModel *model)
    : QSortFilterProxyModel(parent)
    , m_group(nullptr)
    , m_filterCriteria(CFontFilter::CRIT_FAMILY)
    , m_filterWs(0)
    , m_fcQuery(nullptr)
{
    setSourceModel(model);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(0);
    setDynamicSortFilter(false);
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CFontListSortFilterProxy::timeout);
    connect(model, &QAbstractItemModel::layoutChanged, this, &QSortFilterProxyModel::invalidate);
    m_timer->setSingleShot(true);
}

QVariant CFontListSortFilterProxy::data(const QModelIndex &idx, int role) const
{
    if (!idx.isValid()) {
        return QVariant();
    }

    static const int constMaxFiles = 20;

    QModelIndex index(mapToSource(idx));
    CFontModelItem *mi = static_cast<CFontModelItem *>(index.internalPointer());

    if (!mi) {
        return QVariant();
    }

    switch (role) {
    case Qt::ToolTipRole:
        if (CFontFilter::CRIT_FILENAME == m_filterCriteria || CFontFilter::CRIT_LOCATION == m_filterCriteria
            || CFontFilter::CRIT_FONTCONFIG == m_filterCriteria) {
            if (mi->isFamily()) {
                CFamilyItem *fam = static_cast<CFamilyItem *>(index.internalPointer());
                CFontItemCont::ConstIterator it(fam->fonts().begin()), end(fam->fonts().end());
                FileCont allFiles;
                QString tip("<b>" + fam->name() + "</b>");
                bool markMatch(CFontFilter::CRIT_FONTCONFIG == m_filterCriteria);
                tip += "<p style='white-space:pre'><table>";

                for (; it != end; ++it) {
                    allFiles += (*it)->files();
                }

                // qSort(allFiles);
                FileCont::ConstIterator fit(allFiles.begin()), fend(allFiles.end());

                for (int i = 0; fit != fend && i < constMaxFiles; ++fit, ++i) {
                    if (markMatch && m_fcQuery && (*fit).path() == m_fcQuery->file()) {
                        tip += "<tr><td><b>" + Misc::contractHome((*fit).path()) + "</b></td></tr>";
                    } else {
                        tip += "<tr><td>" + Misc::contractHome((*fit).path()) + "</td></tr>";
                    }
                }
                if (allFiles.count() > constMaxFiles) {
                    tip += "<tr><td><i>" + i18n("…plus %1 more", allFiles.count() - constMaxFiles) + "</td></tr>";
                }

                tip += "</table></p>";
                return tip;
            } else {
                CFontItem *font = static_cast<CFontItem *>(index.internalPointer());
                QString tip("<b>" + font->name() + "</b>");
                const FileCont &files(font->files());
                bool markMatch(CFontFilter::CRIT_FONTCONFIG == m_filterCriteria);

                tip += "<p style='white-space:pre'><table>";

                // qSort(files);
                FileCont::ConstIterator fit(files.begin()), fend(files.end());

                for (int i = 0; fit != fend && i < constMaxFiles; ++fit, ++i) {
                    if (markMatch && m_fcQuery && (*fit).path() == m_fcQuery->file()) {
                        tip += "<tr><td><b>" + Misc::contractHome((*fit).path()) + "</b></td></tr>";
                    } else {
                        tip += "<tr><td>" + Misc::contractHome((*fit).path()) + "</td></tr>";
                    }
                }
                if (files.count() > constMaxFiles) {
                    tip += "<tr><td><i>" + i18n("…plus %1 more", files.count() - constMaxFiles) + "</td></tr></li>";
                }

                tip += "</table></p>";
                return tip;
            }
        }
        break;
    case Qt::FontRole:
        if (COL_FONT == index.column() && mi->isSystem()) {
            QFont font;
            font.setItalic(true);
            return font;
        }
        break;
    case Qt::ForegroundRole:
        if (COL_FONT == index.column()
            && ((mi->isFont() && !(static_cast<CFontItem *>(index.internalPointer()))->isEnabled())
                || (mi->isFamily() && CFamilyItem::DISABLED == (static_cast<CFamilyItem *>(index.internalPointer()))->status()))) {
            return KColorScheme(QPalette::Active).foreground(KColorScheme::NegativeText).color();
        }
        break;
    case Qt::DisplayRole:
        if (COL_FONT == index.column()) {
            if (mi->isFamily()) {
                CFamilyItem *fam = static_cast<CFamilyItem *>(index.internalPointer());

                return i18n("%1 [%2]", fam->name(), fam->fontCount());
            } else {
                return (static_cast<CFontItem *>(index.internalPointer()))->style();
            }
        }
        break;
    case Qt::DecorationRole:
        if (mi->isFamily()) {
            CFamilyItem *fam = static_cast<CFamilyItem *>(index.internalPointer());

            switch (index.column()) {
            case COL_STATUS:
                switch (fam->status()) {
                case CFamilyItem::PARTIAL:
                    return QIcon::fromTheme("dialog-ok");
                case CFamilyItem::ENABLED:
                    return QIcon::fromTheme("dialog-ok");
                case CFamilyItem::DISABLED:
                    return QIcon::fromTheme("dialog-cancel");
                }
                break;
            default:
                break;
            }
        } else if (COL_STATUS == index.column()) {
            return QIcon::fromTheme((static_cast<CFontItem *>(index.internalPointer()))->isEnabled() ? "dialog-ok" : "dialog-cancel");
        }
        break;
    case Qt::SizeHintRole:
        if (mi->isFamily()) {
            const int s = KIconLoader::global()->currentSize(KIconLoader::Small);
            return QSize(s, s + 4);
        }
    default:
        break;
    }
    return QVariant();
}

bool CFontListSortFilterProxy::acceptFont(CFontItem *fnt, bool checkFontText) const
{
    if (m_group && (CGroupListItem::ALL != m_group->type() || (!filterText().isEmpty() && checkFontText))) {
        bool fontMatch(!checkFontText);

        if (!fontMatch) {
            switch (m_filterCriteria) {
            case CFontFilter::CRIT_FONTCONFIG:
                fontMatch = m_fcQuery ? fnt->name() == m_fcQuery->font() // || fnt->files().contains(m_fcQuery->file())
                                      : false;
                break;
            case CFontFilter::CRIT_STYLE:
                fontMatch = matchString(fnt->style(), m_filterText);
                break;
            case CFontFilter::CRIT_FOUNDRY: {
                FileCont::ConstIterator it(fnt->files().begin()), end(fnt->files().end());

                for (; it != end && !fontMatch; ++it) {
                    fontMatch = 0 == (*it).foundry().compare(m_filterText, Qt::CaseInsensitive);
                }
                break;
            }
            case CFontFilter::CRIT_FILENAME: {
                FileCont::ConstIterator it(fnt->files().begin()), end(fnt->files().end());

                for (; it != end && !fontMatch; ++it) {
                    QString file(Misc::getFile((*it).path()));
                    int pos(Misc::isHidden(file) ? 1 : 0);

                    if (pos == file.indexOf(m_filterText, pos, Qt::CaseInsensitive)) {
                        fontMatch = true;
                    }
                }
                break;
            }
            case CFontFilter::CRIT_LOCATION: {
                FileCont::ConstIterator it(fnt->files().begin()), end(fnt->files().end());

                for (; it != end && !fontMatch; ++it) {
                    if (0 == Misc::getDir((*it).path()).indexOf(m_filterText, 0, Qt::CaseInsensitive)) {
                        fontMatch = true;
                    }
                }
                break;
            }
            case CFontFilter::CRIT_FILETYPE: {
                FileCont::ConstIterator it(fnt->files().begin()), end(fnt->files().end());
                QStringList::ConstIterator mimeEnd(m_filterTypes.constEnd());

                for (; it != end && !fontMatch; ++it) {
                    QStringList::ConstIterator mime(m_filterTypes.constBegin());

                    for (; mime != mimeEnd; ++mime) {
                        if (Misc::checkExt((*it).path(), *mime)) {
                            fontMatch = true;
                        }
                    }
                }
                break;
            }
            case CFontFilter::CRIT_WS:
                fontMatch = fnt->writingSystems() & m_filterWs;
                break;
            default:
                break;
            }
        }

        return fontMatch && m_group->hasFont(fnt);
    }

    return true;
}

bool CFontListSortFilterProxy::acceptFamily(CFamilyItem *fam) const
{
    CFontItemCont::ConstIterator it(fam->fonts().begin()), end(fam->fonts().end());
    bool familyMatch(CFontFilter::CRIT_FAMILY == m_filterCriteria && matchString(fam->name(), m_filterText));

    for (; it != end; ++it) {
        if (acceptFont(*it, !familyMatch)) {
            return true;
        }
    }
    return false;
}

bool CFontListSortFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index(sourceModel()->index(sourceRow, 0, sourceParent));

    if (index.isValid()) {
        CFontModelItem *mi = static_cast<CFontModelItem *>(index.internalPointer());

        if (mi->isFont()) {
            CFontItem *font = static_cast<CFontItem *>(index.internalPointer());

            return acceptFont(font, !(CFontFilter::CRIT_FAMILY == m_filterCriteria && matchString(font->family(), m_filterText)));
        } else {
            return acceptFamily(static_cast<CFamilyItem *>(index.internalPointer()));
        }
    }

    return false;
}

bool CFontListSortFilterProxy::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (left.isValid() && right.isValid()) {
        CFontModelItem *lmi = static_cast<CFontModelItem *>(left.internalPointer()), *rmi = static_cast<CFontModelItem *>(right.internalPointer());

        if (lmi->isFont() < rmi->isFont()) {
            return true;
        }

        if (lmi->isFont()) {
            CFontItem *lfi = static_cast<CFontItem *>(left.internalPointer()), *rfi = static_cast<CFontItem *>(right.internalPointer());

            if (COL_STATUS == filterKeyColumn()) {
                if (lfi->isEnabled() < rfi->isEnabled() || (lfi->isEnabled() == rfi->isEnabled() && lfi->styleInfo() < rfi->styleInfo())) {
                    return true;
                }
            } else if (lfi->styleInfo() < rfi->styleInfo()) {
                return true;
            }
        } else {
            CFamilyItem *lfi = static_cast<CFamilyItem *>(left.internalPointer()), *rfi = static_cast<CFamilyItem *>(right.internalPointer());

            if (COL_STATUS == filterKeyColumn()) {
                if (lfi->status() < rfi->status() || (lfi->status() == rfi->status() && QString::localeAwareCompare(lfi->name(), rfi->name()) < 0)) {
                    return true;
                }
            } else if (QString::localeAwareCompare(lfi->name(), rfi->name()) < 0) {
                return true;
            }
        }
    }

    return false;
}

void CFontListSortFilterProxy::setFilterGroup(CGroupListItem *grp)
{
    if (grp != m_group) {
        //        bool wasNull=!m_group;

        m_group = grp;

        // if(!(wasNull && m_group && CGroupListItem::ALL==m_group->type()))
        invalidate();
    }
}

void CFontListSortFilterProxy::setFilterText(const QString &text)
{
    if (text != m_filterText) {
        //
        // If we are filtering on file location, then expand ~ to /home/user, etc.
        if (CFontFilter::CRIT_LOCATION == m_filterCriteria && !text.isEmpty() && ('~' == text[0] || '$' == text[0])) {
            if ('~' == text[0]) {
                m_filterText = 1 == text.length() ? QDir::homePath() : QString(text).replace(0, 1, QDir::homePath());
            } else {
                m_filterText = replaceEnvVar(text);
            }
        } else {
            m_filterText = text;
        }

        if (m_filterText.isEmpty()) {
            m_timer->stop();
            timeout();
        } else {
            m_timer->start(CFontFilter::CRIT_FONTCONFIG == m_filterCriteria ? 750 : 400);
        }
    }
}

void CFontListSortFilterProxy::setFilterCriteria(CFontFilter::ECriteria crit, qulonglong ws, const QStringList &ft)
{
    if (crit != m_filterCriteria || ws != m_filterWs || ft != m_filterTypes) {
        m_filterWs = ws;
        m_filterCriteria = crit;
        m_filterTypes = ft;
        if (CFontFilter::CRIT_LOCATION == m_filterCriteria) {
            setFilterText(m_filterText);
        }
        m_timer->stop();
        timeout();
    }
}

void CFontListSortFilterProxy::timeout()
{
    if (CFontFilter::CRIT_FONTCONFIG == m_filterCriteria) {
        int commaPos = m_filterText.indexOf(',');
        QString query(m_filterText);

        if (-1 != commaPos) {
            QString style(query.mid(commaPos + 1));
            query.truncate(commaPos);
            query = query.trimmed();
            query += ":style=";
            style = style.trimmed();
            query += style;
        } else {
            query = query.trimmed();
        }

        if (!m_fcQuery) {
            m_fcQuery = new CFcQuery(this);
            connect(m_fcQuery, &CFcQuery::finished, this, &CFontListSortFilterProxy::fcResults);
        }

        m_fcQuery->run(query);
    } else {
        invalidate();
        Q_EMIT refresh();
    }
}

void CFontListSortFilterProxy::fcResults()
{
    if (CFontFilter::CRIT_FONTCONFIG == m_filterCriteria) {
        invalidate();
        Q_EMIT refresh();
    }
}

CFontListView::CFontListView(QWidget *parent, CFontList *model)
    : QTreeView(parent)
    , m_proxy(new CFontListSortFilterProxy(this, model))
    , m_model(model)
    , m_allowDrops(false)
{
    setModel(m_proxy);
    m_model = model;
    header()->setStretchLastSection(false);
    resizeColumnToContents(COL_STATUS);
    header()->setSectionResizeMode(COL_STATUS, QHeaderView::Fixed);
    header()->setSectionResizeMode(COL_FONT, QHeaderView::Stretch);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSortingEnabled(true);
    sortByColumn(COL_FONT, Qt::AscendingOrder);
    setAllColumnsShowFocus(true);
    setAlternatingRowColors(true);
    setAcceptDrops(true);
    setDropIndicatorShown(false);
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    header()->setSectionsClickable(true);
    header()->setSortIndicatorShown(true);
    connect(this, &QTreeView::collapsed, this, &CFontListView::itemCollapsed);
    connect(header(), &QHeaderView::sectionClicked, this, &CFontListView::setSortColumn);
    connect(m_proxy, &CFontListSortFilterProxy::refresh, this, &CFontListView::refresh);
    connect(m_model, &CFontList::listingPercent, this, &CFontListView::listingPercent);

    setWhatsThis(model->whatsThis());
    header()->setWhatsThis(whatsThis());
    m_menu = new QMenu(this);
    m_deleteAct = m_menu->addAction(QIcon::fromTheme("edit-delete"), i18n("Delete"), this, &CFontListView::del);
    m_menu->addSeparator();
    m_enableAct = m_menu->addAction(QIcon::fromTheme("font-enable"), i18n("Enable"), this, &CFontListView::enable);
    m_disableAct = m_menu->addAction(QIcon::fromTheme("font-disable"), i18n("Disable"), this, &CFontListView::disable);
    if (!Misc::app(KFI_VIEWER).isEmpty()) {
        m_menu->addSeparator();
    }
    m_printAct = Misc::app(KFI_VIEWER).isEmpty() ? nullptr : m_menu->addAction(QIcon::fromTheme("document-print"), i18n("Print…"), this, &CFontListView::print);
    m_viewAct =
        Misc::app(KFI_VIEWER).isEmpty() ? nullptr : m_menu->addAction(QIcon::fromTheme("kfontview"), i18n("Open in Font Viewer"), this, &CFontListView::view);
    m_menu->addSeparator();
    m_menu->addAction(QIcon::fromTheme("view-refresh"), i18n("Reload"), model, &CFontList::load);
}

void CFontListView::getFonts(CJobRunner::ItemList &urls, QStringList &fontNames, QSet<Misc::TFont> *fonts, bool selected, bool getEnabled, bool getDisabled)
{
    QModelIndexList selectedItems(selected ? selectedIndexes() : allIndexes());
    QSet<CFontItem *> usedFonts;
    QModelIndex index;

    Q_FOREACH (index, selectedItems)
        if (index.isValid()) {
            QModelIndex realIndex(m_proxy->mapToSource(index));

            if (realIndex.isValid()) {
                if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                    CFontItem *font = static_cast<CFontItem *>(realIndex.internalPointer());

                    addFont(font, urls, fontNames, fonts, usedFonts, getEnabled, getDisabled);
                } else {
                    CFamilyItem *fam = static_cast<CFamilyItem *>(realIndex.internalPointer());

                    for (int ch = 0; ch < fam->fontCount(); ++ch) {
                        QModelIndex child(m_proxy->mapToSource(index.model()->index(ch, 0, index)));

                        if (child.isValid() && (static_cast<CFontModelItem *>(child.internalPointer()))->isFont()) {
                            CFontItem *font = static_cast<CFontItem *>(child.internalPointer());

                            addFont(font, urls, fontNames, fonts, usedFonts, getEnabled, getDisabled);
                        }
                    }
                }
            }
        }

    fontNames = CFontList::compact(fontNames);
}

QSet<QString> CFontListView::getFiles()
{
    QModelIndexList items(allIndexes());
    QModelIndex index;
    QSet<QString> files;

    Q_FOREACH (index, items)
        if (index.isValid()) {
            QModelIndex realIndex(m_proxy->mapToSource(index));

            if (realIndex.isValid()) {
                if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                    CFontItem *font = static_cast<CFontItem *>(realIndex.internalPointer());

                    FileCont::ConstIterator it(font->files().begin()), end(font->files().end());

                    for (; it != end; ++it) {
                        QStringList assoc;

                        files.insert((*it).path());
                        Misc::getAssociatedFiles((*it).path(), assoc);

                        QStringList::ConstIterator ait(assoc.constBegin()), aend(assoc.constEnd());

                        for (; ait != aend; ++ait) {
                            files.insert(*ait);
                        }
                    }
                }
            }
        }

    return files;
}

void CFontListView::getPrintableFonts(QSet<Misc::TFont> &items, bool selected)
{
    QModelIndexList selectedItems(selected ? selectedIndexes() : allIndexes());
    QModelIndex index;

    Q_FOREACH (index, selectedItems) {
        CFontItem *font = nullptr;

        if (index.isValid() && 0 == index.column()) {
            QModelIndex realIndex(m_proxy->mapToSource(index));

            if (realIndex.isValid()) {
                if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                    font = static_cast<CFontItem *>(realIndex.internalPointer());
                } else {
                    CFamilyItem *fam = static_cast<CFamilyItem *>(realIndex.internalPointer());
                    font = fam->regularFont();
                }
            }
        }

        if (font && !font->isBitmap() && font->isEnabled()) {
            items.insert(Misc::TFont(font->family(), font->styleInfo()));
        }
    }
}

void CFontListView::setFilterGroup(CGroupListItem *grp)
{
    CGroupListItem *oldGrp(m_proxy->filterGroup());

    m_proxy->setFilterGroup(grp);
    m_allowDrops = grp && !grp->isCustom();

    if (!Misc::root()) {
        bool refreshStats(false);

        if (!grp || !oldGrp) {
            refreshStats = true;
        } else {
            // Check to see whether we have changed from listing all fonts,
            // listing just system or listing personal fonts.
            CGroupListItem::EType aType(CGroupListItem::CUSTOM == grp->type() || CGroupListItem::ALL == grp->type()
                                                || CGroupListItem::UNCLASSIFIED == grp->type()
                                            ? CGroupListItem::CUSTOM
                                            : grp->type()),
                bType(CGroupListItem::CUSTOM == oldGrp->type() || CGroupListItem::ALL == oldGrp->type() || CGroupListItem::UNCLASSIFIED == oldGrp->type()
                          ? CGroupListItem::CUSTOM
                          : oldGrp->type());
            refreshStats = aType != bType;
        }

        if (refreshStats) {
            m_model->refresh(!grp || !grp->isPersonal(), !grp || !grp->isSystem());
        }
    }
    // when switching groups, for some reason it is not always sorted.
    setSortingEnabled(true);
}

void CFontListView::listingPercent(int percent)
{
    // when the font list is first loaded, for some reason it is not always sorted.
    // re-enabling sorting here seems to fix the issue - BUG 221610
    if (100 == percent) {
        setSortingEnabled(true);
    }
}

void CFontListView::refreshFilter()
{
    m_proxy->invalidate();
}

void CFontListView::filterText(const QString &text)
{
    m_proxy->setFilterText(text);
}

void CFontListView::filterCriteria(int crit, qulonglong ws, const QStringList &ft)
{
    m_proxy->setFilterCriteria((CFontFilter::ECriteria)crit, ws, ft);
}

void CFontListView::stats(int &enabled, int &disabled, int &partial)
{
    enabled = disabled = partial = 0;

    for (int i = 0; i < m_proxy->rowCount(); ++i) {
        QModelIndex idx(m_proxy->index(i, 0, QModelIndex()));

        if (!idx.isValid()) {
            break;
        }

        QModelIndex sourceIdx(m_proxy->mapToSource(idx));

        if (!sourceIdx.isValid()) {
            break;
        }

        if ((static_cast<CFontModelItem *>(sourceIdx.internalPointer()))->isFamily()) {
            switch ((static_cast<CFamilyItem *>(sourceIdx.internalPointer()))->status()) {
            case CFamilyItem::ENABLED:
                enabled++;
                break;
            case CFamilyItem::DISABLED:
                disabled++;
                break;
            case CFamilyItem::PARTIAL:
                partial++;
                break;
            }
        }
    }
}

void CFontListView::selectedStatus(bool &enabled, bool &disabled)
{
    QModelIndexList selected(selectedIndexes());
    QModelIndex index;

    enabled = disabled = false;

    Q_FOREACH (index, selected) {
        QModelIndex realIndex(m_proxy->mapToSource(index));

        if (realIndex.isValid()) {
            if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFamily()) {
                switch ((static_cast<CFamilyItem *>(realIndex.internalPointer()))->status()) {
                case CFamilyItem::ENABLED:
                    enabled = true;
                    break;
                case CFamilyItem::DISABLED:
                    disabled = true;
                    break;
                case CFamilyItem::PARTIAL:
                    enabled = true;
                    disabled = true;
                    break;
                }
            } else {
                if ((static_cast<CFontItem *>(realIndex.internalPointer()))->isEnabled()) {
                    enabled = true;
                } else {
                    disabled = true;
                }
            }
        }
        if (enabled && disabled) {
            break;
        }
    }
}

QModelIndexList CFontListView::allFonts()
{
    QModelIndexList rv;
    int rowCount(m_proxy->rowCount());

    for (int i = 0; i < rowCount; ++i) {
        QModelIndex idx(m_proxy->index(i, 0, QModelIndex()));
        int childRowCount(m_proxy->rowCount(idx));

        for (int j = 0; j < childRowCount; ++j) {
            QModelIndex child(m_proxy->index(j, 0, idx));

            if (child.isValid()) {
                rv.append(m_proxy->mapToSource(child));
            }
        }
    }

    return rv;
}

void CFontListView::selectFirstFont()
{
    if (0 == selectedIndexes().count()) {
        for (int i = 0; i < NUM_COLS; ++i) {
            QModelIndex idx(m_proxy->index(0, i, QModelIndex()));

            if (idx.isValid()) {
                selectionModel()->select(idx, QItemSelectionModel::Select);
            }
        }
    }
}

void CFontListView::setSortColumn(int col)
{
    if (col != m_proxy->filterKeyColumn()) {
        m_proxy->setFilterKeyColumn(col);
        m_proxy->invalidate();
    }
}

void CFontListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected, deselected);
    if (m_model->slowUpdates()) {
        return;
    }
    Q_EMIT itemsSelected(getSelectedItems());
}

QModelIndexList CFontListView::getSelectedItems()
{
    //
    // Go through current selection, and for any 'font' items that are selected,
    // ensure 'family' item is not...
    QModelIndexList selectedItems(selectedIndexes()), deselectList;
    QModelIndex index;
    QSet<CFontModelItem *> selectedFamilies;
    bool en(false), dis(false);

    Q_FOREACH (index, selectedItems)
        if (index.isValid()) {
            QModelIndex realIndex(m_proxy->mapToSource(index));

            if (realIndex.isValid()) {
                if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                    CFontItem *font = static_cast<CFontItem *>(realIndex.internalPointer());

                    if (font->isEnabled()) {
                        en = true;
                    } else {
                        dis = true;
                    }
                    if (!selectedFamilies.contains(font->parent())) {
                        selectedFamilies.insert(font->parent());

                        for (int i = 0; i < NUM_COLS; ++i) {
                            deselectList.append(m_proxy->mapFromSource(m_model->createIndex(font->parent()->rowNumber(), i, font->parent())));
                        }
                    }
                } else {
                    switch ((static_cast<CFamilyItem *>(realIndex.internalPointer()))->status()) {
                    case CFamilyItem::ENABLED:
                        en = true;
                        break;
                    case CFamilyItem::DISABLED:
                        dis = true;
                        break;
                    case CFamilyItem::PARTIAL:
                        en = dis = true;
                        break;
                    }
                }
            }
        }

    if (deselectList.count()) {
        Q_FOREACH (index, deselectList)
            selectionModel()->select(index, QItemSelectionModel::Deselect);
    }

    QModelIndexList sel;
    QSet<void *> pointers;
    selectedItems = selectedIndexes();
    Q_FOREACH (index, selectedItems) {
        QModelIndex idx(m_proxy->mapToSource(index));

        if (!pointers.contains(idx.internalPointer())) {
            pointers.insert(idx.internalPointer());
            sel.append(idx);
        }
    }

    return sel;
}

void CFontListView::itemCollapsed(const QModelIndex &idx)
{
    if (idx.isValid()) {
        QModelIndex index(m_proxy->mapToSource(idx));

        if (index.isValid() && (static_cast<CFontModelItem *>(index.internalPointer()))->isFamily()) {
            CFamilyItem *fam = static_cast<CFamilyItem *>(index.internalPointer());
            CFontItemCont::ConstIterator it(fam->fonts().begin()), end(fam->fonts().end());

            for (; it != end; ++it) {
                for (int i = 0; i < NUM_COLS; ++i) {
                    selectionModel()->select(m_proxy->mapFromSource(m_model->createIndex((*it)->rowNumber(), i, *it)), QItemSelectionModel::Deselect);
                }
            }
        }
    }
}

static bool isScalable(const QString &str)
{
    QByteArray cFile(QFile::encodeName(str));

    return Misc::checkExt(cFile, "ttf") || Misc::checkExt(cFile, "otf") || Misc::checkExt(cFile, "ttc") || Misc::checkExt(cFile, "pfa")
        || Misc::checkExt(cFile, "pfb");
}

void CFontListView::view()
{
    // Number of fonts user has selected, before we ask if they really want to view them all...
    static const int constMaxBeforePrompt = 10;

    QModelIndexList selectedItems(selectedIndexes());
    QModelIndex index;
    QSet<CFontItem *> fonts;

    Q_FOREACH (index, selectedItems) {
        QModelIndex realIndex(m_proxy->mapToSource(index));

        if (realIndex.isValid()) {
            if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                CFontItem *font(static_cast<CFontItem *>(realIndex.internalPointer()));

                fonts.insert(font);
            } else {
                CFontItem *font((static_cast<CFamilyItem *>(realIndex.internalPointer()))->regularFont());

                if (font) {
                    fonts.insert(font);
                }
            }
        }
    }

    if (fonts.count()
        && (fonts.count() < constMaxBeforePrompt
            || KMessageBox::Yes == KMessageBox::questionYesNo(this, i18n("Open all %1 fonts in font viewer?", fonts.count())))) {
        QSet<CFontItem *>::ConstIterator it(fonts.begin()), end(fonts.end());
        QStringList args;

        for (; it != end; ++it) {
            QString file;
            int index(0);

            if (!(*it)->isEnabled()) {
                // For a disabled font, we need to find the first scalable font entry in its file list...
                FileCont::ConstIterator fit((*it)->files().begin()), fend((*it)->files().end());

                for (; fit != fend; ++fit) {
                    if (isScalable((*fit).path())) {
                        file = (*fit).path();
                        index = (*fit).index();
                        break;
                    }
                }
                if (file.isEmpty()) {
                    file = (*it)->fileName();
                    index = (*it)->index();
                }
            }
            args << FC::encode((*it)->family(), (*it)->styleInfo(), file, index).url();
        }

        QProcess::startDetached(Misc::app(KFI_VIEWER), args);
    }
}

QModelIndexList CFontListView::allIndexes()
{
    QModelIndexList rv;
    int rowCount(m_proxy->rowCount());

    for (int i = 0; i < rowCount; ++i) {
        QModelIndex idx(m_proxy->index(i, 0, QModelIndex()));
        int childRowCount(m_proxy->rowCount(idx));

        rv.append(idx);

        for (int j = 0; j < childRowCount; ++j) {
            QModelIndex child(m_proxy->index(j, 0, idx));

            if (child.isValid()) {
                rv.append(child);
            }
        }
    }

    return rv;
}

void CFontListView::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList indexes(selectedIndexes());

    if (indexes.count()) {
        QMimeData *data = model()->mimeData(indexes);
        if (!data) {
            return;
        }

        QModelIndex index(m_proxy->mapToSource(indexes.first()));
        const char *icon = "application-x-font-pcf";

        if (index.isValid()) {
            CFontItem *font = (static_cast<CFontModelItem *>(index.internalPointer()))->isFont()
                ? static_cast<CFontItem *>(index.internalPointer())
                : (static_cast<CFamilyItem *>(index.internalPointer()))->regularFont();

            if (font && !font->isBitmap()) {
                //                 if("application/x-font-type1"==font->mimetype())
                //                     icon="application-x-font-type1";
                //                 else
                icon = "application-x-font-ttf";
            }
        }

        QPoint hotspot;
        QPixmap pix = QIcon::fromTheme(icon).pixmap(KIconLoader::SizeMedium);

        hotspot.setX(0); // pix.width()/2);
        hotspot.setY(0); // pix.height()/2);

        QDrag *drag = new QDrag(this);
        drag->setPixmap(pix);
        drag->setMimeData(data);
        drag->setHotSpot(hotspot);
        drag->exec(supportedActions);
    }
}

void CFontListView::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_allowDrops && event->mimeData()->hasFormat("text/uri-list")) { // "application/x-kde-urilist" ??
        event->acceptProposedAction();
    }
}

void CFontListView::dropEvent(QDropEvent *event)
{
    if (m_allowDrops && event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();

        QList<QUrl> urls(event->mimeData()->urls());
        QList<QUrl>::ConstIterator it(urls.begin()), end(urls.end());
        QSet<QUrl> kurls;
        QMimeDatabase db;

        for (; it != end; ++it) {
            QMimeType mime = db.mimeTypeForUrl(*it);

            Q_FOREACH (const QString &fontMime, CFontList::fontMimeTypes) {
                if (mime.inherits(fontMime)) {
                    kurls.insert(*it);
                    break;
                }
            }
        }

        if (!kurls.isEmpty()) {
            Q_EMIT fontsDropped(kurls);
        }
    }
}

void CFontListView::contextMenuEvent(QContextMenuEvent *ev)
{
    bool valid(indexAt(ev->pos()).isValid());

    m_deleteAct->setEnabled(valid);

    bool en(false), dis(false);
    QModelIndexList selectedItems(selectedIndexes());
    QModelIndex index;

    Q_FOREACH (index, selectedItems) {
        QModelIndex realIndex(m_proxy->mapToSource(index));

        if (realIndex.isValid()) {
            if ((static_cast<CFontModelItem *>(realIndex.internalPointer()))->isFont()) {
                if ((static_cast<CFontItem *>(realIndex.internalPointer())->isEnabled())) {
                    en = true;
                } else {
                    dis = true;
                }
            } else {
                switch ((static_cast<CFamilyItem *>(realIndex.internalPointer()))->status()) {
                case CFamilyItem::ENABLED:
                    en = true;
                    break;
                case CFamilyItem::DISABLED:
                    dis = true;
                    break;
                case CFamilyItem::PARTIAL:
                    en = dis = true;
                    break;
                }
            }
        }
        if (en && dis) {
            break;
        }
    }

    m_enableAct->setEnabled(dis);
    m_disableAct->setEnabled(en);
    if (m_printAct) {
        m_printAct->setEnabled(en | dis);
    }
    if (m_viewAct) {
        m_viewAct->setEnabled(en | dis);
    }
    m_menu->popup(ev->globalPos());
}

bool CFontListView::viewportEvent(QEvent *event)
{
    executeDelayedItemsLayout();
    return QTreeView::viewportEvent(event);
}

}
