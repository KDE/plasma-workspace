/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "icons.h"

#include <QHash>
#include <QRegularExpression>

QString Icons::actionIcon(const QString &actionName)
{
    QString icon;

    QString action = actionName;

    if (action.isEmpty()) {
        return icon;
    }

    static const QHash<QString, QString> s_icons{
        {QStringLiteral("new"), QStringLiteral("document-new")}, // appmenu-gtk-module "New"
        {QStringLiteral("image-new"), QStringLiteral("document-new")}, // Gimp "New" item
        {QStringLiteral("adddirect"), QStringLiteral("document-new")}, // LibreOffice "New" item
        {QStringLiteral("filenew"), QStringLiteral("document-new")}, // Pluma "New" item
        {QStringLiteral("new-window"), QStringLiteral("window-new")},
        {QStringLiteral("newwindow"), QStringLiteral("window-new")},
        {QStringLiteral("yelp-window-new"), QStringLiteral("window-new")}, // Gnome help
        {QStringLiteral("new-tab"), QStringLiteral("tab-new")},
        {QStringLiteral("open"), QStringLiteral("document-open")},
        {QStringLiteral("open-location"), QStringLiteral("document-open-remote")},
        {QStringLiteral("openremote"), QStringLiteral("document-open-remote")},
        {QStringLiteral("save"), QStringLiteral("document-save")},
        {QStringLiteral("save-as"), QStringLiteral("document-save-as")},
        {QStringLiteral("saveas"), QStringLiteral("document-save-as")},
        {QStringLiteral("save-all"), QStringLiteral("document-save-all")},
        {QStringLiteral("saveall"), QStringLiteral("document-save-all")},
        {QStringLiteral("import"), QStringLiteral("document-import")},
        {QStringLiteral("export"), QStringLiteral("document-export")},
        {QStringLiteral("exportto"), QStringLiteral("document-export")}, // LibreOffice
        {QStringLiteral("exporttopdf"), QStringLiteral("viewpdf")}, // LibreOffice, the icon it uses but the name is quite random
        {QStringLiteral("webhtml"), QStringLiteral("text-html")}, // LibreOffice
        {QStringLiteral("printpreview"), QStringLiteral("document-print-preview")},
        {QStringLiteral("print-preview"), QStringLiteral("document-print-preview")},
        {QStringLiteral("print"), QStringLiteral("document-print")},
        {QStringLiteral("print-gtk"), QStringLiteral("document-print")}, // Gimp
        {QStringLiteral("mail-image"), QStringLiteral("mail-message-new")}, // Gimp
        {QStringLiteral("sendmail"), QStringLiteral("mail-message-new")}, // LibreOffice
        {QStringLiteral("sendviabluetooth"), QStringLiteral("preferences-system-bluetooth")}, // LibreOffice
        {QStringLiteral("sendviabluetooth"), QStringLiteral("preferences-system-bluetooth")}, // LibreOffice
        {QStringLiteral("document-properties"), QStringLiteral("document-properties")},
        {QStringLiteral("close"), QStringLiteral("document-close")}, // appmenu-gtk-module "Close"
        {QStringLiteral("closedoc"), QStringLiteral("document-close")},
        {QStringLiteral("close-all"), QStringLiteral("document-close")},
        {QStringLiteral("closeall"), QStringLiteral("document-close")},
        {QStringLiteral("closewin"), QStringLiteral("window-close")}, // LibreOffice
        {QStringLiteral("quit"), QStringLiteral("application-exit")},

        {QStringLiteral("undo"), QStringLiteral("edit-undo")},
        {QStringLiteral("redo"), QStringLiteral("edit-redo")},
        {QStringLiteral("revert"), QStringLiteral("document-revert")},
        {QStringLiteral("cut"), QStringLiteral("edit-cut")},
        {QStringLiteral("copy"), QStringLiteral("edit-copy")},
        {QStringLiteral("paste"), QStringLiteral("edit-paste")},
        {QStringLiteral("duplicate"), QStringLiteral("edit-duplicate")},

        {QStringLiteral("preferences"), QStringLiteral("settings-configure")},
        {QStringLiteral("optionstreedialog"), QStringLiteral("settings-configure")}, // LibreOffice
        {QStringLiteral("keyboard-shortcuts"), QStringLiteral("configure-shortcuts")},

        {QStringLiteral("fullscreen"), QStringLiteral("view-fullscreen")},

        {QStringLiteral("find"), QStringLiteral("edit-find")},
        {QStringLiteral("searchfind"), QStringLiteral("edit-find")},
        {QStringLiteral("replace"), QStringLiteral("edit-find-replace")},
        {QStringLiteral("searchreplace"), QStringLiteral("edit-find-replace")}, // LibreOffice
        {QStringLiteral("searchdialog"), QStringLiteral("edit-find-replace")}, // LibreOffice
        {QStringLiteral("find-replace"), QStringLiteral("edit-find-replace")}, // Inkscape
        {QStringLiteral("select-all"), QStringLiteral("edit-select-all")},
        {QStringLiteral("selectall"), QStringLiteral("edit-select-all")},
        {QStringLiteral("select-none"), QStringLiteral("edit-select-invert")},
        {QStringLiteral("select-invert"), QStringLiteral("edit-select-invert")},
        {QStringLiteral("invert-selection"), QStringLiteral("edit-select-invert")}, // Inkscape
        {QStringLiteral("check-spelling"), QStringLiteral("tools-check-spelling")},
        {QStringLiteral("set-language"), QStringLiteral("set-language")},

        {QStringLiteral("increasesize"), QStringLiteral("zoom-in")},
        {QStringLiteral("decreasesize"), QStringLiteral("zoom-out")},
        {QStringLiteral("zoom-in"), QStringLiteral("zoom-in")},
        {QStringLiteral("zoom-out"), QStringLiteral("zoom-out")},
        {QStringLiteral("zoomfit"), QStringLiteral("zoom-fit-best")},
        {QStringLiteral("zoom-fit-in"), QStringLiteral("zoom-fit-best")},
        {QStringLiteral("show-guides"), QStringLiteral("show-guides")},
        {QStringLiteral("show-grid"), QStringLiteral("show-grid")},

        {QStringLiteral("rotateclockwise"), QStringLiteral("object-rotate-right")},
        {QStringLiteral("rotatecounterclockwise"), QStringLiteral("object-rotate-left")},
        {QStringLiteral("fliphorizontally"), QStringLiteral("object-flip-horizontal")},
        {QStringLiteral("image-flip-horizontal"), QStringLiteral("object-flip-horizontal")},
        {QStringLiteral("flipvertically"), QStringLiteral("object-flip-vertical")},
        {QStringLiteral("image-flip-vertical"), QStringLiteral("object-flip-vertical")},
        {QStringLiteral("image-scale"), QStringLiteral("transform-scale")},

        {QStringLiteral("bold"), QStringLiteral("format-text-bold")},
        {QStringLiteral("italic"), QStringLiteral("format-text-italic")},
        {QStringLiteral("underline"), QStringLiteral("format-text-underline")},
        {QStringLiteral("strikeout"), QStringLiteral("format-text-strikethrough")},
        {QStringLiteral("superscript"), QStringLiteral("format-text-superscript")},
        {QStringLiteral("subscript"), QStringLiteral("format-text-subscript")},
        // "grow" is a bit unspecific to always set it to "grow font", so use the exact ID here
        {QStringLiteral(".uno:Grow"), QStringLiteral("format-font-size-more")}, // LibreOffice
        {QStringLiteral(".uno:Shrink"), QStringLiteral("format-font-size-less")}, // LibreOffice
        // also a bit unspecific?
        {QStringLiteral("alignleft"), QStringLiteral("format-justify-left")},
        {QStringLiteral("alignhorizontalcenter"), QStringLiteral("format-justify-center")},
        {QStringLiteral("alignright"), QStringLiteral("format-justify-right")},
        {QStringLiteral("alignjustified"), QStringLiteral("format-justify-fill")},
        {QStringLiteral("incrementindent"), QStringLiteral("format-indent-more")},
        {QStringLiteral("decrementindent"), QStringLiteral("format-indent-less")},
        {QStringLiteral("defaultbullet"), QStringLiteral("format-list-unordered")}, // LibreOffice
        {QStringLiteral("defaultnumbering"), QStringLiteral("format-list-ordered")}, // LibreOffice

        {QStringLiteral("sortascending"), QStringLiteral("view-sort-ascending")},
        {QStringLiteral("sortdescending"), QStringLiteral("view-sort-descending")},

        {QStringLiteral("autopilotmenu"), QStringLiteral("tools-wizard")}, // LibreOffice

        {QStringLiteral("layers-new"), QStringLiteral("layer-new")},
        {QStringLiteral("layers-duplicate"), QStringLiteral("layer-duplicate")},
        {QStringLiteral("layers-delete"), QStringLiteral("layer-delete")},
        {QStringLiteral("layers-anchor"), QStringLiteral("anchor")},

        {QStringLiteral("slideshow"), QStringLiteral("media-playback-start")}, // Gwenview uses this icon for that
        {QStringLiteral("playvideo"), QStringLiteral("media-playback-start")},

        {QStringLiteral("addtags"), QStringLiteral("tag-new")},
        {QStringLiteral("newevent"), QStringLiteral("appointment-new")},

        {QStringLiteral("previous-document"), QStringLiteral("go-previous")},
        {QStringLiteral("prevphoto"), QStringLiteral("go-previous")},
        {QStringLiteral("next-document"), QStringLiteral("go-next")},
        {QStringLiteral("nextphoto"), QStringLiteral("go-next")},

        {QStringLiteral("redeye"), QStringLiteral("redeyes")},
        {QStringLiteral("crop"), QStringLiteral("transform-crop")},
        {QStringLiteral("move"), QStringLiteral("transform-move")},
        {QStringLiteral("rotate"), QStringLiteral("transform-rotate")},
        {QStringLiteral("scale"), QStringLiteral("transform-scale")},
        {QStringLiteral("shear"), QStringLiteral("transform-shear")},
        {QStringLiteral("flip"), QStringLiteral("object-flip-horizontal")},
        {QStringLiteral("flag"), QStringLiteral("flag-red")}, // is there a "mark" or "important" icon that isn't email?

        {QStringLiteral("tools-measure"), QStringLiteral("measure")},
        {QStringLiteral("tools-text"), QStringLiteral("draw-text")},
        {QStringLiteral("tools-color-picker"), QStringLiteral("color-picker")},
        {QStringLiteral("tools-paintbrush"), QStringLiteral("draw-brush")},
        {QStringLiteral("tools-eraser"), QStringLiteral("draw-eraser")},
        {QStringLiteral("tools-paintbrush"), QStringLiteral("draw-brush")},

        {QStringLiteral("help"), QStringLiteral("help-contents")},
        {QStringLiteral("helpindex"), QStringLiteral("help-contents")},
        {QStringLiteral("contents"), QStringLiteral("help-contents")},
        {QStringLiteral("helpcontents"), QStringLiteral("help-contents")},
        {QStringLiteral("context-help"), QStringLiteral("help-whatsthis")},
        {QStringLiteral("extendedhelp"), QStringLiteral("help-whatsthis")}, // LibreOffice
        {QStringLiteral("helpreportproblem"), QStringLiteral("tools-report-bug")},
        {QStringLiteral("sendfeedback"), QStringLiteral("tools-report-bug")}, // LibreOffice
        {QStringLiteral("about"), QStringLiteral("help-about")},

        {QStringLiteral("emptytrash"), QStringLiteral("trash-empty")},
        {QStringLiteral("movetotrash"), QStringLiteral("user-trash-symbolic")},

        // Gnome help
        {QStringLiteral("yelp-application-larger-text"), QStringLiteral("format-font-size-more")},
        {QStringLiteral("yelp-application-smaller-text"), QStringLiteral("format-font-size-less")}, // LibreOffice

        // LibreOffice documents in its New menu
        {QStringLiteral("private:factory/swriter"), QStringLiteral("application-vnd.oasis.opendocument.text")},
        {QStringLiteral("private:factory/scalc"), QStringLiteral("application-vnd.oasis.opendocument.spreadsheet")},
        {QStringLiteral("private:factory/simpress"), QStringLiteral("application-vnd.oasis.opendocument.presentation")},
        {QStringLiteral("private:factory/sdraw"), QStringLiteral("application-vnd.oasis.opendocument.graphics")},
        {QStringLiteral("private:factory/swriter/web"), QStringLiteral("text-html")},
        {QStringLiteral("private:factory/smath"), QStringLiteral("application-vnd.oasis.opendocument.formula")},
    };

    // Sometimes we get additional arguments (?slot=123) we don't care about
    const int questionMarkIndex = action.indexOf(QLatin1Char('?'));
    if (questionMarkIndex > -1) {
        action.truncate(questionMarkIndex);
    }

    icon = s_icons.value(action);

    if (icon.isEmpty()) {
        const int dotIndex = action.indexOf(QLatin1Char('.')); // app., win., or unity. prefix

        QString prefix;
        if (dotIndex > -1) {
            prefix = action.left(dotIndex);

            action = action.mid(dotIndex + 1);
        }

        // appmenu-gtk-module
        if (prefix == QLatin1String("unity")) {
            // Remove superfluous hyphens added by appmenu-gtk-module
            // First remove multiple subsequent ones
            static QRegularExpression subsequentHyphenRegExp(QStringLiteral("-{2,}"));
            action.replace(subsequentHyphenRegExp, QStringLiteral("-"));

            // now we can be sure we only have a single hyphen at the start or end, remove it if needed
            if (action.startsWith(QLatin1Char('-'))) {
                action.remove(0, 1);
            }
            if (action.endsWith(QLatin1Char('-'))) {
                action.chop(1);
            }

            // It also turns accelerators (&) into hyphens, so remove any hyphen that comes before
            // a lower-case letter ("mid sentence"), e.g. "P-references"
            static QRegularExpression strayHyphenRegExp(QStringLiteral("-(?=[a-z]+)"));
            action.remove(strayHyphenRegExp);
        }

        icon = s_icons.value(action);
    }

    if (icon.isEmpty()) {
        static const auto s_dup1Prefix = QStringLiteral("dup:1:"); // can it be dup2 also?
        if (action.startsWith(s_dup1Prefix)) {
            action = action.mid(s_dup1Prefix.length());
        }

        static const auto s_unoPrefix = QStringLiteral(".uno:"); // LibreOffice with appmenu-gtk
        if (action.startsWith(s_unoPrefix)) {
            action = action.mid(s_unoPrefix.length());
        }

        // LibreOffice's "Open" entry is always "OpenFromAppname" so we just chop that off
        if (action.startsWith(QLatin1String("OpenFrom"))) {
            action.truncate(4); // basically "Open"
        }

        icon = s_icons.value(action);
    }

    if (icon.isEmpty()) {
        static const auto s_commonPrefix = QStringLiteral("Common");
        if (action.startsWith(s_commonPrefix)) {
            action = action.mid(s_commonPrefix.length());
        }

        icon = s_icons.value(action);
    }

    if (icon.isEmpty()) {
        static const auto s_prefixes = QStringList{
            // Gimp with appmenu-gtk
            QStringLiteral("file-"),
            QStringLiteral("edit-"),
            QStringLiteral("view-"),
            QStringLiteral("image-"),
            QStringLiteral("layers-"),
            QStringLiteral("colors-"),
            QStringLiteral("tools-"),
            QStringLiteral("plug-in-"),
            QStringLiteral("windows-"),
            QStringLiteral("dialogs-"),
            QStringLiteral("help-"),
        };

        for (const QString &prefix : s_prefixes) {
            if (action.startsWith(prefix)) {
                action = action.mid(prefix.length());
                break;
            }
        }

        icon = s_icons.value(action);
    }

    if (icon.isEmpty()) {
        action = action.toLower();
        icon = s_icons.value(action);
    }

    if (icon.isEmpty()) {
        static const auto s_prefixes = QStringList{
            // Pluma with appmenu-gtk
            QStringLiteral("file"),
            QStringLiteral("edit"),
            QStringLiteral("view"),
            QStringLiteral("help"),
        };

        for (const QString &prefix : s_prefixes) {
            if (action.startsWith(prefix)) {
                action = action.mid(prefix.length());
                break;
            }
        }

        icon = s_icons.value(action);
    }

    return icon;
}
