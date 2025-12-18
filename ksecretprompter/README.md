<!--
    SPDX-License-Identifier: CC0-1.0
    SPDX-FileCopyrightText: Harald Sitter <sitter@kde.org>
-->

# KSecretPrompter
A secret (aka password) prompter framework for KDE Plasma. Specifically geared towards the [oo7 secret service implementation](https://github.com/bilelmoussaoui/oo7).

## About
KSecretPrompter provides KDE Plasma with a password prompting framework. Its API is accessed over DBus and is available as xml definitions in this repository.

The basic API flow is to create a [src/org.kde.secretprompter.xml](Request) object on your dbus address under any path.
Then you call one of the [prompt methods](src/org.kde.secretprompter.xm) with that path as argument.
KSecretPrompter will display a UI prompt to the user and once that is done call back to you on your object path.

## Building
The easiest way to make changes and test them during development is to build it with [https://community.kde.org/Get_Involved/development/Build_software_with_kdesrc-build](kdesrc-build).
