/*
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
//draw a circle with an antialiased border
//innerRadius = size of the inner circle with contents
//outerRadius = size of the border
//blend = area to blend between two colours
//all sizes are normalised so 0.5 == half the width of the texture

//if copying into another project don't forget to connect themeChanged to update()
//but in SDDM that's a bit pointless

#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec4 colorBorder;
};
layout(binding = 1) uniform sampler2D source;
layout(location = 0) out vec4 fragColor;


const highp float blend = 0.01;
const highp float innerRadius = 0.47;
const highp float outerRadius = 0.49;
const lowp vec4 colorEmpty = vec4(0.0, 0.0, 0.0, 0.0);

void main() {
    lowp vec4 colorSource = texture(source, qt_TexCoord0);

    highp vec2 m = qt_TexCoord0 - vec2(0.5, 0.5);
    highp float dist = sqrt(m.x * m.x + m.y * m.y);

    if (dist < innerRadius)
        fragColor = colorSource;
    else if (dist < innerRadius + blend)
        fragColor = mix(colorSource, colorBorder, ((dist - innerRadius) / blend));
    else if (dist < outerRadius)
        fragColor = colorBorder;
    else if (dist < outerRadius + blend)
        fragColor = mix(colorBorder, colorEmpty, ((dist - outerRadius) / blend));
    else
        fragColor = colorEmpty;

    fragColor = fragColor * qt_Opacity;
}
