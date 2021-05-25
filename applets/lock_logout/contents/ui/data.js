var data = [{
    icon: "system-lock-screen",
    operation: "lock",
    configKey: "lockScreen",
    tooltip_mainText: i18n("Lock"),
    tooltip_subText: i18n("Lock the screen"),
    requires: "Lock"
}, {
    icon: "system-switch-user",
    operation: "switchUser",
    configKey: "switchUser",
    tooltip_mainText: i18n("Switch user"),
    tooltip_subText: i18n("Start a parallel session as a different user")
}, {
    icon: "system-shutdown",
    operation: "requestShutdown",
    configKey: "requestShutDown",
    tooltip_mainText: i18n("Shutdown…"),
    tooltip_subText: i18n("Turn off the computer"),
    requires: "Shutdown"
}, {
    icon: "system-reboot",
    operation: "requestReboot",
    configKey: "requestReboot",
    tooltip_mainText: i18n("Restart…"),
    tooltip_subText: i18n("Reboot the computer"),
    requires: "Reboot"
}, {
    icon: "system-log-out",
    operation: "requestLogout",
    configKey: "requestLogout",
    tooltip_mainText: i18n("Logout…"),
    tooltip_subText: i18n("End the session"),
    requires: "Logout"
}, {
    icon: "system-suspend",
    operation: "suspend",
    configKey: "suspendToRam",
    tooltip_mainText: i18nc("Suspend to RAM", "Sleep"),
    tooltip_subText: i18n("Sleep (suspend to RAM)"),
    requires: "Suspend"
}, {
    icon: "system-suspend-hibernate",
    operation: "hibernate",
    configKey: "suspendToDisk",
    tooltip_mainText: i18n("Hibernate"),
    tooltip_subText: i18n("Hibernate (suspend to disk)"),
    requires: "Hibernate"
}]
