#!/bin/env python3

import os
import sys
import xml.dom.minidom
from xml.dom.minidom import parse

########
# This application loops through installed applets and extracts the config XML file
# Output is then presented in mediawiki format for copying and pasting to https://userbase.kde.org/KDE_System_Administration/PlasmaDesktopScripting#Configuration_Keys

# app should be kept flexible enough to port to a different format in future


#set plasmoid installation path manually if applicable
xdg_data_dir = ""

if not xdg_data_dir:
    xdg_data_dirs = os.getenv("XDG_DATA_DIRS").split(":")
    xdg_data_dirs.append("/usr/share")
    xdg_data_dir = xdg_data_dirs[0]



root = xdg_data_dir + "/plasma/plasmoids"

plasmoids = os.listdir(root)
plasmoids.sort()
for plasmoid in plasmoids:
    configPath = "/contents/config/main.xml"
    path  = root + "/" + plasmoid + configPath
    try:
        dom = xml.dom.minidom.parse(path).documentElement

        print ("===" + plasmoid + "===")
        for group in dom.getElementsByTagName("group"):
            groupName = group.getAttribute("name")
            print ("======" + groupName + "======")
            for entry in group.getElementsByTagName("entry"):
                name = entry.getAttribute("name")
                type = entry.getAttribute("type")
                default = ""
                description = ""

                if entry.hasAttribute("hidden") and entry.getAttribute("hidden") == "true":
                    continue

                defaultTags = entry.getElementsByTagName("default")
                if (defaultTags.length > 0 and defaultTags[0].childNodes.length > 0):
                    default = defaultTags[0].childNodes[0].data

                if (default == ""):
                    if (type == "Bool"):
                        default = "false"
                    elif (type == "Int"):
                        default = "0"
                    elif (type == "StringList"):
                        default = "empty list"
                    elif (type == "String"):
                        default = "empty string"
                    else:
                        default = "null"

                labelTags = entry.getElementsByTagName("label")
                if (labelTags.length > 0 and labelTags[0].childNodes.length > 0):
                    description = labelTags[0].childNodes[0].data


                print ("* '''%s''' (''%s'', default ''%s'') %s" % (name , type, default, description))
    except IOError:
        sys.stderr.write("No config in " + plasmoid +"\n")
    #abort on other errors so we can find them
