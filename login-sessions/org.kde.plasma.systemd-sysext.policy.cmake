<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD polkit Policy Configuration 1.0//EN"
"http://www.freedesktop.org/software/polkit/policyconfig-1.dtd">
<!--
  SPDX-License-Identifier: CC0-1.0
-->
<policyconfig>

  <vendor>KDE</vendor>
  <vendor_url>https://www.kde.org/</vendor_url>

  <action id="org.kde.plasma.systemd-sysext.pkexec.merge">
    <description>Run programs as another user</description>
    <message>Authentication is required to run a program as another user</message>
    <defaults>
      <allow_any>no</allow_any>
      <allow_inactive>no</allow_inactive>
      <allow_active>yes</allow_active>
    </defaults>
    <annotate key="org.freedesktop.policykit.exec.path">@CMAKE_INSTALL_FULL_LIBEXECDIR@/systemd-sysext-merge</annotate>
  </action>

  <action id="org.kde.plasma.systemd-sysext.pkexec.unmerge">
    <description>Run programs as another user</description>
    <message>Authentication is required to run a program as another user</message>
    <defaults>
      <allow_any>no</allow_any>
      <allow_inactive>no</allow_inactive>
      <allow_active>yes</allow_active>
    </defaults>
    <annotate key="org.freedesktop.policykit.exec.path">@CMAKE_INSTALL_FULL_LIBEXECDIR@/systemd-sysext-unmerge</annotate>
  </action>
</policyconfig>
