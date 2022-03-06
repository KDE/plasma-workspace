#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KLazyLocalizedString>
#include <KLocalizedString>
#include <QLatin1String>
#include <kio/udsentry.h>

#define KFI_NAME "kfontinst"
#define KFI_CATALOGUE KFI_NAME

#define KFI_INSTALLER QLatin1String("kfontinst")
#define KFI_VIEWER QLatin1String("kfontview")
#define KFI_PRINTER QLatin1String("kfontprint"), "libexec"

#define KFI_PRINT_GROUP "Print"
#define KFI_KIO_FONTS_PROTOCOL "fonts"
constexpr const auto KFI_KIO_FONTS_USER = kli18n("Personal");
constexpr const auto KFI_KIO_FONTS_SYS = kli18n("System");
constexpr const auto KFI_KIO_FONTS_ALL = kli18n("All");
#define KFI_KIO_NO_CLEAR "noclear"
#define KFI_KIO_TIMEOUT "timeout"
#define KFI_KIO_FACE "face"

#define KFI_IFACE "org.kde.fontinst"

#define KFI_FILE_QUERY "file"
#define KFI_NAME_QUERY "name"
#define KFI_STYLE_QUERY "style"
#define KFI_MIME_QUERY "mime"

#define KFI_SYS_USER "root"

constexpr const auto KFI_AUTHINF_USER = kli18n("Administrator");
#define KFI_UI_CFG_FILE KFI_NAME "uirc"
#define KFI_ROOT_CFG_DIR "/etc/fonts/"
#define KFI_DEFAULT_SYS_FONTS_FOLDER "/usr/local/share/fonts/"

#define KFI_NO_STYLE_INFO 0xFFFFFFFF
#define KFI_NAME_KEY "Name="
#define KFI_STYLE_KEY "Style="
#define KFI_PATH_KEY "Path="
#define KFI_FACE_KEY "Face="

#define KFI_FONTS_PACKAGE ".fonts.zip"
#define KFI_FONTS_PACKAGE_LEN 10
#define KFI_GROUPS_FILE "fontgroups.xml"
#define KFI_TMP_DIR_PREFIX "kfi"

#define KFI_NULL_SETTING 0xFF

namespace KFI
{
// KIO::special
enum ESpecial {
    SPECIAL_RESCAN = 0,
    SPECIAL_CONFIGURE,
};

// UDS_EXTRA entries...
enum EUdsExtraEntries {
    UDS_EXTRA_FC_STYLE = (((KIO::UDSEntry::UDS_EXTRA | KIO::UDSEntry::UDS_STRING) ^ KIO::UDSEntry::UDS_STRING) | KIO::UDSEntry::UDS_NUMBER) + 1,
    UDS_EXTRA_FILE_NAME = KIO::UDSEntry::UDS_EXTRA + 2,
    UDS_EXTRA_FILE_FACE = KIO::UDSEntry::UDS_EXTRA + 3,
};

}

// Font name...
constexpr const auto KFI_WEIGHT_THIN = kli18n("Thin");
constexpr const auto KFI_WEIGHT_EXTRALIGHT = kli18n("Extra Light");
constexpr const auto KFI_WEIGHT_ULTRALIGHT = kli18n("Ultra Light");
constexpr const auto KFI_WEIGHT_LIGHT = kli18n("Light");
constexpr const auto KFI_WEIGHT_REGULAR = kli18n("Regular");
constexpr const auto KFI_WEIGHT_NORMAL = kli18n("Normal");
constexpr const auto KFI_WEIGHT_MEDIUM = kli18n("Medium");
constexpr const auto KFI_WEIGHT_DEMIBOLD = kli18n("Demi Bold");
constexpr const auto KFI_WEIGHT_SEMIBOLD = kli18n("Semi Bold");
constexpr const auto KFI_WEIGHT_BOLD = kli18n("Bold");
constexpr const auto KFI_WEIGHT_EXTRABOLD = kli18n("Extra Bold");
constexpr const auto KFI_WEIGHT_ULTRABOLD = kli18n("Ultra Bold");
constexpr const auto KFI_WEIGHT_BLACK = kli18n("Black");
constexpr const auto KFI_WEIGHT_HEAVY = kli18n("Heavy");

constexpr const auto KFI_SLANT_ROMAN = kli18n("Roman");
constexpr const auto KFI_SLANT_ITALIC = kli18n("Italic");
constexpr const auto KFI_SLANT_OBLIQUE = kli18n("Oblique");

constexpr const auto KFI_WIDTH_ULTRACONDENSED = kli18n("Ultra Condensed");
constexpr const auto KFI_WIDTH_EXTRACONDENSED = kli18n("Extra Condensed");
constexpr const auto KFI_WIDTH_CONDENSED = kli18n("Condensed");
constexpr const auto KFI_WIDTH_SEMICONDENSED = kli18n("Semi Condensed");
constexpr const auto KFI_WIDTH_NORMAL = kli18n("Normal");
constexpr const auto KFI_WIDTH_SEMIEXPANDED = kli18n("Semi Expanded");
constexpr const auto KFI_WIDTH_EXPANDED = kli18n("Expanded");
constexpr const auto KFI_WIDTH_EXTRAEXPANDED = kli18n("Extra Expanded");
constexpr const auto KFI_WIDTH_ULTRAEXPANDED = kli18n("Ultra Expanded");

constexpr const auto KFI_SPACING_MONO = kli18n("Monospaced");
constexpr const auto KFI_SPACING_CHARCELL = kli18n("Charcell");
constexpr const auto KFI_SPACING_PROPORTIONAL = kli18n("Proportional");

constexpr const auto KFI_UNKNOWN_FOUNDRY = kli18n("Unknown");
