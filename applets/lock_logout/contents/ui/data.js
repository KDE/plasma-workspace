var data = [{
    icon: "system-lock-screen",
    operation: "lock",
    tooltip_mainText: i18n("Lock"),
    tooltip_subText: i18n("Lock the screen"),
    requires: "Lock"
}, {
    icon: "system-switch-user",
    operation: "switchUser",
    tooltip_mainText: i18n("Switch user"),
    tooltip_subText: i18n("Start a parallel session as a different user")
}, {
    icon: "system-shutdown",
    operation: "requestShutdown",
    tooltip_mainText: i18n("Leave..."),
    tooltip_subText: i18n("Log out, turn off or restart the computer"),
    requires: "Shutdown"
}, {
    icon: "system-suspend",
    operation: "suspend",
    tooltip_mainText: i18nc("Suspend to RAM", "Sleep"),
    tooltip_subText: i18n("Sleep (suspend to RAM)"),
    requires: "Suspend"
}, {
    icon: "system-suspend-hibernate",
    operation: "hibernate",
    tooltip_mainText: i18n("Hibernate"),
    tooltip_subText: i18n("Hibernate (suspend to disk)"),
    requires: "Hibernate"
}]
