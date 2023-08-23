/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
    SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    mat4 colorMatrix;
};
layout(binding = 1) uniform mediump sampler2D source;
layout(location = 0) out vec4 fragColor;

void main(void)
{
    mediump vec4 tex = texture(source, qt_TexCoord0);
    fragColor = tex * colorMatrix * qt_Opacity;
}
