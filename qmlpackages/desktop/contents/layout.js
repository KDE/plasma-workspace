
var panel = new Panel
panel.screen = 0
panel.location = 'top'
panel.addWidget("org.kde.taskmanager")

for (var i = 0; i < screenCount; ++i) {
    var desktop = new Activity
    desktop.name = i18n("Desktop")
    desktop.screen = i
    desktop.wallpaperPlugin = 'org.kde.image'

    desktop.addWidget("org.kde.testapplet")
    var testComponents = desktop.addWidget("org.kde.testcomponentsapplet")
}
