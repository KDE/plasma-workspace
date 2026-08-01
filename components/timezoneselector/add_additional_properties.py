#
# SPDX-FileCopyrightText: 2026 James Graham <james.h.graham@protonmail.com>
#
# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
#

import os
import json
from shapely.geometry import shape

def add_additional_properties(filename):
    filename = 'timezones.json'

    with open(filename, 'r') as f:
        data = json.load(f)
        for feature in data["features"]:
            polygon = shape(feature)
            feature["properties"]["centroid"] = polygon.centroid.coords[0]
            feature["properties"]["bounds"] = polygon.bounds

    os.remove(filename)
    with open(filename, 'w') as f:
        f.write('{"type":"FeatureCollection", "features": [\n')
        first = True
        for feature in data["features"]:
            if first:
                first = False
            else:
                f.write(',\n')
            json.dump(feature, f, separators=(',', ':'))
        f.write('\n]}')

filename = 'timezones.json'

add_additional_properties(filename)
