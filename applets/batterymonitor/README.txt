Obviously, this is a battery monitor applet for Plasma.

The catch of those four .svg files is the following:

battery-inkscape.svg is an so-called "Inkscape SVG", it's the one
		you want to edit end export to
battery.svg as plain .svg file.

Likewise, for the Oxygen theme.

This file contains various layers:

* Battery The "background", an empty battery
* AcAdapter A flash, saying "Ac is plugged in"
* Fill10 10% filled, red
* Fill20 20% filled, orange
* Fill20 30% filled, green (and so on...)
* Fill30
* Fill40
* Fill50
* Fill60
* Fill70
* Fill80
* Fill90
* Fill010
* Shadow An Oxygen-style shadow under the battery

The Battery layer is always rendered as the first step. On top
of that, we render only one of the FillN layers. If the AC
Adapter is plugged in, we paint that layer on top of the cake.
It makes no difference when we paint the Shadow, there's no
overlap between shadow and other layers.
